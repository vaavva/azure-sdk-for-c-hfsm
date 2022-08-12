/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hub_hfsm.c
 * @brief HFSM for Azure IoT Hub Operations.
 *
 * @details Implements connectivity and fault handling for an IoT Hub Client
 */

#include <stdint.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/iot/internal/az_iot_hub_hfsm.h>

#include <azure/core/_az_cfg.h>

static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event);

static az_hfsm_return_type idle(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type started(az_hfsm* me, az_hfsm_event event);

static az_hfsm_return_type connecting(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type connected(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type disconnecting(az_hfsm* me, az_hfsm_event event);

static az_hfsm_return_type subscribing(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type subscribed(az_hfsm* me, az_hfsm_event event);

static az_hfsm_state_handler _get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == idle || child_state == started)
  {
    parent_state = root;
  }
  else if (child_state == connecting || child_state == connected || child_state == disconnecting)
  {
    parent_state = started;
  }
  else if (child_state == subscribing || child_state == subscribed)
  {
    parent_state = connected;
  }
  else
  {
    // Unknown state.
    az_platform_critical_error();
    parent_state = NULL;
  }

  return parent_state;
}

AZ_NODISCARD az_hfsm_iot_hub_policy_options az_hfsm_iot_hub_policy_options_default()
{
  return (az_hfsm_iot_hub_policy_options){ ._reserved = 0 };
}

AZ_NODISCARD az_result az_hfsm_iot_hub_policy_initialize(
    az_hfsm_iot_hub_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_hfsm_policy* outbound_policy,
    az_iot_hub_client* hub_client,
    az_hfsm_iot_hub_policy_options const* options)
{
  policy->_internal.options = options == NULL ? az_hfsm_iot_hub_policy_options_default() : *options;

  policy->_internal.policy.outbound = outbound_policy;
  policy->_internal.policy.inbound = inbound_policy;

  policy->_internal.policy.pipeline = pipeline;

  policy->_internal.hub_client = hub_client;

  _az_RETURN_IF_FAILED(az_hfsm_init((az_hfsm*)policy, root, _get_parent));
  az_hfsm_transition_substate((az_hfsm*)policy, root, idle);

  return AZ_OK;
}

static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/root"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op.
      break;

    case AZ_HFSM_EVENT_EXIT:
    default:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_EXIT))
      {
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_iot_hub/root: PANIC!"));
      }

      az_platform_critical_error();
      break;
  }

  return ret;
}

AZ_INLINE void _hub_invalid_state(az_hfsm_iot_hub_policy* me)
{
  az_hfsm_event_data_error error_data
      = (az_hfsm_event_data_error){ .error_type = AZ_ERROR_HFSM_INVALID_STATE };

  az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_EVENT_ERROR, .data = &error_data });
}

AZ_INLINE void _hub_connect(az_hfsm_iot_hub_policy* me, az_hfsm_iot_hub_connect_data* data)
{
  // TODO
  (void)me;
  (void)data;
}

static az_hfsm_return_type idle(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_hfsm_iot_hub_policy* this_policy = (az_hfsm_iot_hub_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_IOT_HUB_DISCONNECT_REQ:
    case AZ_IOT_HUB_TELEMETRY_REQ:
    case AZ_IOT_HUB_METHODS_RSP:
      _hub_invalid_state(this_policy);
      break;

    case AZ_IOT_HUB_CONNECT_REQ:
      az_hfsm_transition_peer(me, idle, started);
      az_hfsm_transition_substate(me, started, connecting);
      _hub_connect(this_policy, (az_hfsm_iot_hub_connect_data*)event.data);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_return_type started(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // TODO
      break;

    case AZ_HFSM_EVENT_EXIT:
      // TODO
      break;

    default:
      // TODO
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_return_type connecting(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/connecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // TODO
      break;

    case AZ_HFSM_EVENT_EXIT:
      // TODO
      break;

    default:
      // TODO
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_return_type connected(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/connected"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // TODO
      break;

    case AZ_HFSM_EVENT_EXIT:
      // TODO
      break;

    default:
      // TODO
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_return_type disconnecting(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/disconnecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // TODO
      break;

    case AZ_HFSM_EVENT_EXIT:
      // TODO
      break;

    default:
      // TODO
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_return_type subscribing(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/connected/subscribing"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // TODO
      break;

    case AZ_HFSM_EVENT_EXIT:
      // TODO
      break;

    default:
      // TODO
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_return_type subscribed(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/connected/subscribed"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // TODO
      break;

    case AZ_HFSM_EVENT_EXIT:
      // TODO
      break;

    default:
      // TODO
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}
