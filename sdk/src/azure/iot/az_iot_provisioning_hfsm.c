/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief HFSM for Azure IoT Operations.
 *
 * @details Implements the state machine for for Azure IoT Device Provisioning.
 */

#include <inttypes.h>
#include <stdint.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/internal/az_iot_provisioning_hfsm.h>

#include <azure/core/_az_cfg.h>
_
static az_hfsm_return_type _root(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _idle(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _started(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _connecting(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _connected(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _disconnecting(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _subscribing(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _subscribed(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _start_register(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _wait_register(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _delay(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type _start_query(az_hfsm* me, az_hfsm_event event);

// Hardcoded Azure IoT hierarchy structure
static az_hfsm_state_handler _get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if ((child_state == root))
  {
    parent_state = NULL;
  }
  else if ((child_state == idle) || (child_state == started))
  {
    parent_state = root;
  }
  else if ((child_state == connecting) 
        || (child_state == connected) 
        || (child_state == disconnecting))
  {
    parent_state = started;
  }
  else if ((child_state == subscribing) || (child_state == subscribed))
  {
    parent_state = connected;
  }
  else if ((child_state == start_register) 
        || (child_state == wait_register)
        || (child_state == delay)
        || (child_state == start_query))
  {
    parent_state = subscribed;
  }
  else
  {
    // Unknown state.
    az_platform_critical_error();
    parent_state = NULL;
  }

  return parent_state;
}

// AzureIoTProvisioning
static az_hfsm_return_type _root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/"));
      }

      this_iot_hfsm->_internal.use_secondary_credentials = false;
      break;

    case AZ_HFSM_EVENT_EXIT:
    case AZ_HFSM_EVENT_TIMEOUT:
    default:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/"));
      }

      az_platform_critical_error();
      break;
  }

  return ret;
}

// AzureIoTProvisioning/Idle
static az_hfsm_return_type _idle(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/idle"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/idle"));
      }
      break;

    case AZ_HFSM_IOT_EVENT_START:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_IOT_START))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_IOT_START, AZ_SPAN_FROM_STR("provisioning/idle"));
      }


      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started
static az_hfsm_return_type _started(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started/Connecting
static az_hfsm_return_type _connecting(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started/connecting"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started/connecting"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started/Disconnecting
static az_hfsm_return_type _disconnecting(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started/disconnecting"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started/disconnecting"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started/Connected
static az_hfsm_return_type _connected(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started/connected"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started/connected"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started/Connected/Subscribing
static az_hfsm_return_type _subscribing(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribing"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribing"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started/Connected/Subscribed
static az_hfsm_return_type _subscribed(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started/Connected/Subscribed/Start_Register
static az_hfsm_return_type _start_register(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed/start_register"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed/start_register"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started/Connected/Subscribed/Wait_Register
static az_hfsm_return_type _wait_register(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed/wait_register"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed/wait_register"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started/Connected/Subscribed/Delay
static az_hfsm_return_type _delay(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed/delay"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed/delay"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoTProvisioning/Started/Connected/Subscribed/Query
static az_hfsm_return_type _query(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_iot_provisioning_hfsm_type* this_iot_hfsm = (az_iot_provisioning_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed/query"));
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("provisioning/started/connected/subscribed/query"));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

AZ_NODISCARD az_result az_iot_provisioning_hfsm_initialize(
  az_iot_provisioning_hfsm_type* provisioning_hfsm, 
  az_hfsm_dispatch* iot_hfsm);
{
  az_result ret;

  provisioning_hfsm->_internal.iot_hfsm = iot_hfsm;
  ret = az_hfsm_init((az_hfsm*)(provisioning_hfsm), _root, _get_parent);

  // Transition to initial state.
  if (az_result_succeeded(ret))
  {
    az_hfsm_transition_substate((az_hfsm*)(provisioning_hfsm), _root, idle);
  }
  
  return ret;
}
