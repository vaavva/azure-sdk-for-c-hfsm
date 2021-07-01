/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief HFSM for Azure IoT Operations.
 * 
 * @details Implements fault handling for Device Provisioning + IoT Hub operations
 */

#include <stdint.h>
#include <inttypes.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/iot/internal/az_iot_hfsm.h>

const az_hfsm_event az_hfsm_event_az_iot_start = { AZ_IOT_START, NULL };
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
const az_hfsm_event az_hfsm_event_az_iot_provisioning_done = { AZ_IOT_PROVISIONING_DONE, NULL };
#endif

static int32_t azure_iot(az_hfsm* me, az_hfsm_event event);
static int32_t idle(az_hfsm* me, az_hfsm_event event);
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
static int32_t provisioning(az_hfsm* me, az_hfsm_event event);
#endif
static int32_t hub(az_hfsm* me, az_hfsm_event event);

// Hardcoded Azure IoT hierarchy structure
static az_hfsm_state_handler azure_iot_hfsm_get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if ((child_state == azure_iot))
  {
    parent_state = NULL;
  }
  else if (
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED    
      (child_state == provisioning) ||
#endif
      (child_state == hub) || (child_state == idle)
    )
  {
    parent_state = azure_iot;
  }
  else 
  {
    // Unknown state.
    az_iot_hfsm_pal_critical_error(NULL);
    parent_state = NULL;
  }

  return parent_state;
}

// AzureIoT
static int32_t azure_iot(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = 0;
  int64_t operation_msec;

  az_iot_hfsm_type* this_iot_hfsm = (az_iot_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTER))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTER, AZ_SPAN_FROM_STR("az_iot_hfsm/azure_iot"));
      }

      this_iot_hfsm->_use_secondary_credentials = false;
      break;

    case AZ_IOT_ERROR:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ERROR))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ERROR, AZ_SPAN_FROM_STR("az_iot_hfsm/azure_iot"));
      }

      az_platform_clock_msec(&operation_msec);
      operation_msec -= this_iot_hfsm->_start_time_msec;

      if (this_iot_hfsm->_retry_attempt < INT16_MAX)
      {
        this_iot_hfsm->_retry_attempt++;
      }

      az_iot_hfsm_event_data_error* error_data = (az_iot_hfsm_event_data_error*)(event.data);

      bool should_retry = false;
      if (error_data->type == AZ_IOT_ERROR_TYPE_SERVICE)
      {
        should_retry = az_iot_status_retriable(error_data->iot_status);
      }
      else if (error_data->type == AZ_IOT_ERROR_TYPE_NETWORK)
      {
        should_retry = true;
      }

      if (should_retry)
      {
        int32_t random_jitter_msec = az_iot_hfsm_pal_get_random_jitter_msec(me);

        int32_t retry_delay_msec = az_iot_calculate_retry_delay(
          (int32_t)operation_msec, 
          this_iot_hfsm->_retry_attempt,
          AZ_IOT_HFSM_MIN_RETRY_DELAY_MSEC,
          AZ_IOT_HFSM_MAX_RETRY_DELAY_MSEC,
          random_jitter_msec);

        ret = az_platform_timer_start(this_iot_hfsm->_timer_handle, retry_delay_msec);
      }
      else
      {
        _az_PRECONDITION(error_data->type == AZ_IOT_ERROR_TYPE_SECURITY);
        this_iot_hfsm->_use_secondary_credentials = !this_iot_hfsm->_use_secondary_credentials;
        az_platform_clock_msec(&this_iot_hfsm->_start_time_msec);

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
          ret = az_hfsm_send_event(this_iot_hfsm->_provisioning_hfsm, az_hfsm_event_az_iot_start);
          ret = az_hfsm_transition_substate(me, azure_iot, provisioning);
#else
          ret = az_hfsm_send_event(this_iot_hfsm->_iothub_hfsm, az_hfsm_event_az_iot_start);
          ret = az_hfsm_transition_substate(me, azure_iot, hub);
#endif
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
    case AZ_HFSM_EVENT_ERROR:
    case AZ_HFSM_EVENT_TIMEOUT:
    default:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ERROR))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ERROR, AZ_SPAN_FROM_STR("az_iot_hfsm/azure_iot"));
      }

      az_iot_hfsm_pal_critical_error(me);
      break;
  }

  return ret;
}

static int32_t idle(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = 0;
  az_iot_hfsm_type* this_iot_hfsm = (az_iot_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTER))
      {
        // AZ_LOG_HFSM_ENTER - common events (enter/exit/timeout/error) are standardized across state machines.
        // The string span should be standardized to represent the full path to the state (including any superstates and the state machine as root)
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTER, AZ_SPAN_FROM_STR("az_iot_hfsm/azure_iot/idle"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("az_iot_hfsm/azure_iot/idle"));
      }
      break;
    
    case AZ_IOT_START:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_IOT_START))
      {
        _az_LOG_WRITE(AZ_LOG_IOT_START, AZ_SPAN_FROM_STR("az_iot_hfsm/azure_iot/idle"));
      }

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
      if(az_hfsm_send_event(this_iot_hfsm->_provisioning_hfsm, az_hfsm_event_az_iot_start))
      {
        ret = az_hfsm_send_event(me, az_hfsm_error_event);
      }
      else
      {
        ret = az_hfsm_transition_peer(me, idle, provisioning);
      }
#else
      if(az_hfsm_send_event(this_iot_hfsm->_iothub_hfsm, az_hfsm_event_az_iot_start))
      {
        ret = az_hfsm_send_event(me, az_hfsm_error_event);
      }
      else
      {
        ret = az_hfsm_transition_peer(me, idle, hub);
      }
#endif
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
// AzureIoT/Provisioning
static int32_t provisioning(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = 0;
  az_iot_hfsm_type* this_iot_hfsm = (az_iot_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      LogInfo( ("provisioning: Entry") );
      this_iot_hfsm->_retry_attempt = 0;
      this_iot_hfsm->_start_time_msec = az_hfsm_pal_timer_get_milliseconds();
      this_iot_hfsm->_timer_handle = az_hfsm_pal_timer_create(me);
      break;

    case AZ_HFSM_EVENT_EXIT:
      LogInfo( ("provisioning: Exit") );
      az_hfsm_pal_timer_destroy(me, this_iot_hfsm->_timer_handle);
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      LogInfo( ("provisioning: Timeout") );
      this_iot_hfsm->_start_time_msec = az_hfsm_pal_timer_get_milliseconds();
      ret = az_hfsm_send_event(this_iot_hfsm->_provisioning_hfsm, az_hfsm_event_az_iot_start);
      break;
    
    case AZ_IOT_PROVISIONING_DONE:
      LogInfo( ("provisioning: Done") );
      ret = az_hfsm_send_event(this_iot_hfsm->_iothub_hfsm, az_hfsm_event_az_iot_start);
      ret = az_hfsm_transition_peer(me, provisioning, hub);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}
#endif

// AzureIoT/Hub
static int32_t hub(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = 0;
  az_iot_hfsm_type* this_iot_hfsm = (az_iot_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      LogInfo( ("hub: Entry") );
      this_iot_hfsm->_retry_attempt = 0;
      this_iot_hfsm->_start_time_msec = az_hfsm_pal_timer_get_milliseconds();
      this_iot_hfsm->_timer_handle = az_hfsm_pal_timer_create(me);
      break;

    case AZ_HFSM_EVENT_EXIT:
      LogInfo( ("hub: Exit") );
      az_hfsm_pal_timer_destroy(me, this_iot_hfsm->_timer_handle);
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      LogInfo( ("hub: Timeout") );
      this_iot_hfsm->_start_time_msec = az_hfsm_pal_timer_get_milliseconds();
      ret = az_hfsm_send_event(this_iot_hfsm->_iothub_hfsm, az_hfsm_event_az_iot_start);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

/**
 * @brief 
 * 
 * @param iot_hfsm 
 * @param provisioning_hfsm 
 * @param hub_hfsm 
 * @return int32_t 
 */
int32_t az_iot_hfsm_initialize(
  az_iot_hfsm_type* iot_hfsm, 
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
  az_hfsm* provisioning_hfsm, 
#endif
  az_hfsm* hub_hfsm)
{
  int32_t ret = 0;

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
  iot_hfsm->_provisioning_hfsm = provisioning_hfsm;
#endif

  iot_hfsm->_iothub_hfsm = hub_hfsm;
  ret = az_hfsm_init((az_hfsm*)(iot_hfsm), azure_iot, azure_iot_hfsm_get_parent);

  // Transition to initial state.
  if (!ret)
  {
    ret = az_hfsm_transition_substate((az_hfsm*)(iot_hfsm), azure_iot, idle);
  }

  return ret;
}
