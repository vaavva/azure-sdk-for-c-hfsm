/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief HFSM for Azure IoT Operations.
 *
 * @details Implements fault handling for Device Provisioning + IoT Hub operations
 * @see az_iot_hfsm.puml
 */

#include <inttypes.h>
#include <stdint.h>

#include <azure/core/az_result.h>
#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/internal/az_iot_hfsm.h>

#include <azure/core/_az_cfg.h>

const az_hfsm_event az_hfsm_event_az_iot_start = { AZ_HFSM_IOT_EVENT_START, NULL };
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
const az_hfsm_event az_hfsm_event_az_iot_provisioning_done
    = { AZ_HFSM_IOT_EVENT_PROVISIONING_DONE, NULL };
#endif

static az_result root(az_hfsm* me, az_hfsm_event event);
static az_result idle(az_hfsm* me, az_hfsm_event event);
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
static az_result provisioning(az_hfsm* me, az_hfsm_event event);
#endif
static az_result hub(az_hfsm* me, az_hfsm_event event);

// Hardcoded Azure IoT hierarchy structure
static az_hfsm_state_handler azure_iot_hfsm_get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if ((child_state == root))
  {
    parent_state = NULL;
  }
  else if (
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
      (child_state == provisioning) ||
#endif
      (child_state == hub) || (child_state == idle))
  {
    parent_state = root;
  }
  else
  {
    // Unknown state.
    az_platform_critical_error();
    parent_state = NULL;
  }

  return parent_state;
}

// AzureIoT
static az_result root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  int64_t operation_msec;

  az_iot_hfsm_type* this_iot_hfsm = (az_iot_hfsm_type*)me;
  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hfsm/root"));
  }

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      this_iot_hfsm->_internal.use_secondary_credentials = false;
      break;

    case AZ_HFSM_IOT_EVENT_ERROR:
    {
      az_result az_ret = az_platform_clock_msec(&operation_msec);
      _az_PRECONDITION(az_result_succeeded(az_ret));

      operation_msec -= this_iot_hfsm->_internal.start_time_msec;

      if (this_iot_hfsm->_internal.retry_attempt < INT16_MAX)
      {
        this_iot_hfsm->_internal.retry_attempt++;
      }

      az_iot_hfsm_event_data_error* error_data = (az_iot_hfsm_event_data_error*)(event.data);

      bool should_retry = ((error_data->error.error_type == AZ_ERROR_IOT_NETWORK)
                           && (this_iot_hfsm->_internal.retry_attempt <= AZ_IOT_HFSM_MAX_HUB_RETRY))
          || ((error_data->error.error_type == AZ_ERROR_IOT_SERVICE)
              && az_iot_status_retriable(error_data->iot_status));

      if (should_retry)
      {
        int32_t random_jitter_msec;
        az_ret = az_platform_get_random(&random_jitter_msec);
        _az_PRECONDITION(az_result_succeeded(az_ret));
        _az_PRECONDITION(random_jitter_msec > 0);

        random_jitter_msec %= AZ_IOT_HFSM_MAX_RETRY_JITTER_MSEC;

        int32_t retry_delay_msec = az_iot_calculate_retry_delay(
            (int32_t)operation_msec,
            this_iot_hfsm->_internal.retry_attempt,
            AZ_IOT_HFSM_MIN_RETRY_DELAY_MSEC,
            AZ_IOT_HFSM_MAX_RETRY_DELAY_MSEC,
            random_jitter_msec);

        ret = az_platform_timer_start(this_iot_hfsm->_internal.timer_handle, retry_delay_msec);
      }
      else
      {
        _az_PRECONDITION(error_data->error.error_type == AZ_ERROR_IOT_SECURITY);

        this_iot_hfsm->_internal.use_secondary_credentials
            = !this_iot_hfsm->_internal.use_secondary_credentials;
        az_ret = az_platform_clock_msec(&this_iot_hfsm->_internal.start_time_msec);
        _az_PRECONDITION(az_result_succeeded(az_ret));

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
        az_ret = az_hfsm_dispatch_post_event(
            this_iot_hfsm->_internal.provisioning_hfsm, &this_iot_hfsm->_internal.start_event);
#else
        az_ret = az_hfsm_dispatch_post_event(
            this_iot_hfsm->_internal.iothub_hfsm, &this_iot_hfsm->_internal.start_event);
#endif

        if (az_result_succeeded(az_ret))
        {
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
          az_hfsm_transition_substate(me, root, provisioning);
#else
          az_hfsm_transition_substate(me, root, hub);
#endif
        }
        else
        {
          az_hfsm_event_data_error e = { .error_type = az_ret };
          az_hfsm_send_event(me, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &e });
        }
      }
      break;
    }
    case AZ_HFSM_EVENT_ERROR:
      // Exitting the top-level state to cause a critical error.
      az_hfsm_send_event(me, az_hfsm_event_exit);
      break;

    case AZ_HFSM_EVENT_EXIT:
    case AZ_HFSM_EVENT_TIMEOUT:
    default:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_EXIT))
      {
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_iot_hfsm/root: PANIC!"));
      }

      az_platform_critical_error();
      break;
  }

  return ret;
}

static az_result idle(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_iot_hfsm_type* this_iot_hfsm = (az_iot_hfsm_type*)me;
  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hfsm/root/idle"));
  }

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_HFSM_IOT_EVENT_START:
    {
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
      az_result az_ret = az_hfsm_dispatch_post_event(
          this_iot_hfsm->_internal.provisioning_hfsm, &az_hfsm_event_az_iot_start);
#else
      az_hfsm_dispatch_post_event(
          this_iot_hfsm->_internal.iothub_hfsm, &az_hfsm_event_az_iot_start);
#endif

      if (az_result_succeeded(az_ret))
      {
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
        az_hfsm_transition_peer(me, idle, provisioning);
#else
        az_hfsm_transition_peer(me, idle, hub);
#endif
      }
      else
      {
        az_hfsm_event_data_error e = { .error_type = az_ret };
        az_hfsm_send_event(me, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &e });
      }
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
// AzureIoT/Provisioning
static az_result provisioning(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_iot_hfsm_type* this_iot_hfsm = (az_iot_hfsm_type*)me;
  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hfsm/root/provisioning"));
  }

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      this_iot_hfsm->_internal.retry_attempt = 0;
      az_result az_ret = az_platform_clock_msec(&this_iot_hfsm->_internal.start_time_msec);
      _az_PRECONDITION(az_result_succeeded(az_ret));

      az_ret = az_hfsm_timer_create(
          me, &this_iot_hfsm->_internal.timer_data, &this_iot_hfsm->_internal.timer_handle);
      if (az_result_failed(az_ret))
      {
        az_hfsm_event_data_error d = { az_ret };
        az_hfsm_send_event(me, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &d });
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      az_platform_timer_destroy(this_iot_hfsm->_internal.timer_handle);
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      az_ret = az_platform_clock_msec(&this_iot_hfsm->_internal.start_time_msec);
      _az_PRECONDITION(az_result_succeeded(az_ret));

      az_ret = az_hfsm_dispatch_post_event(
          this_iot_hfsm->_internal.provisioning_hfsm, &az_hfsm_event_az_iot_start);
      if (az_result_failed(az_ret))
      {
        az_hfsm_event_data_error d = { az_ret };
        az_hfsm_send_event(me, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &d });
      }

      break;

    case AZ_HFSM_IOT_EVENT_PROVISIONING_DONE:
      az_ret = az_hfsm_dispatch_post_event(
          this_iot_hfsm->_internal.hub_hfsm, &az_hfsm_event_az_iot_start);
      if (az_result_failed(az_ret))
      {
        az_hfsm_event_data_error d = { az_ret };
        az_hfsm_send_event(me, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &d });
      }
      else
      {
        az_hfsm_transition_peer(me, provisioning, hub);
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}
#endif

// AzureIoT/Hub
static az_result hub(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_iot_hfsm_type* this_iot_hfsm = (az_iot_hfsm_type*)me;
  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hfsm/root/hub"));
  }

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      this_iot_hfsm->_internal.retry_attempt = 0;

      az_result az_ret = az_platform_clock_msec(&this_iot_hfsm->_internal.start_time_msec);
      _az_PRECONDITION(az_result_succeeded(az_ret));

      az_ret = az_hfsm_timer_create(
          me, &this_iot_hfsm->_internal.timer_data, &this_iot_hfsm->_internal.timer_handle);
      if (az_result_failed(az_ret))
      {
        az_hfsm_event_data_error d = { az_ret };
        az_hfsm_send_event(me, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &d });
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      az_platform_timer_destroy(this_iot_hfsm->_internal.timer_handle);
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      az_ret = az_platform_clock_msec(&this_iot_hfsm->_internal.start_time_msec);
      _az_PRECONDITION(az_result_succeeded(az_ret));

      az_ret = az_hfsm_dispatch_post_event(
          this_iot_hfsm->_internal.hub_hfsm, &az_hfsm_event_az_iot_start);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

AZ_NODISCARD az_result az_iot_hfsm_initialize(
    az_iot_hfsm_type* iot_hfsm,
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
    az_hfsm_dispatch* provisioning_hfsm,
#endif
    az_hfsm_dispatch* hub_hfsm)
{
  az_result ret;

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
  iot_hfsm->_internal.provisioning_hfsm = provisioning_hfsm;
#endif

  iot_hfsm->_internal.hub_hfsm = hub_hfsm;
  ret = az_hfsm_init((az_hfsm*)(iot_hfsm), root, azure_iot_hfsm_get_parent);

  // Transition to initial state.
  if (az_result_succeeded(ret))
  {
    az_hfsm_transition_substate((az_hfsm*)(iot_hfsm), root, idle);
  }
  
  // Configure the AZ_HFSM_IOT_EVENT_START data.
  iot_hfsm->_internal.start_event.type = AZ_HFSM_IOT_EVENT_START;
  iot_hfsm->_internal.start_event.data = &iot_hfsm->_internal.use_secondary_credentials;

  return ret;
}
