/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_provisioning_hfsm.c
 * @brief HFSM for Azure IoT Provisioning Operations.
 *
 * @details Implements connectivity and fault handling for an IoT Hub Client
 */

#include <stdint.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/iot/internal/az_iot_provisioning_hfsm.h>

#include <azure/core/_az_cfg.h>

static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event);

static az_hfsm_return_type idle(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type started(az_hfsm* me, az_hfsm_event event);

static az_hfsm_return_type connecting(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type connected(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type disconnecting(az_hfsm* me, az_hfsm_event event);

static az_hfsm_return_type subscribing(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type subscribed(az_hfsm* me, az_hfsm_event event);

static az_hfsm_return_type start_register(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type wait_register(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type delay(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type query(az_hfsm* me, az_hfsm_event event);

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
  else if (
      child_state == start_register || child_state == wait_register || child_state == delay
      || child_state == query)
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

AZ_NODISCARD az_hfsm_iot_provisioning_policy_options
az_hfsm_iot_provisioning_policy_options_default()
{
  return (az_hfsm_iot_provisioning_policy_options){ .port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT };
}

AZ_NODISCARD az_result az_hfsm_iot_provisioning_policy_initialize(
    az_hfsm_iot_provisioning_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_hfsm_policy* outbound_policy,
    az_iot_provisioning_client* provisioning_client,
    az_hfsm_iot_provisioning_policy_options const* options)
{
  policy->_internal.options
      = options == NULL ? az_hfsm_iot_provisioning_policy_options_default() : *options;

  policy->_internal.policy.outbound = outbound_policy;
  policy->_internal.policy.inbound = inbound_policy;
  policy->_internal.policy.pipeline = pipeline;

  policy->_internal.provisioning_client = provisioning_client;

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
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/root"));
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
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_iot_provisioning/root: PANIC!"));
      }

      az_platform_critical_error();
      break;
  }

  return ret;
}

AZ_INLINE void _dps_connect(
    az_hfsm_iot_provisioning_policy* me,
    az_hfsm_iot_provisioning_register_data* data)
{
  az_hfsm_mqtt_connect_data connect_data;
  connect_data.host = me->_internal.provisioning_client->_internal.global_device_endpoint;
  connect_data.port = me->_internal.options.port;

  if (data->auth_type == AZ_HFSM_IOT_AUTH_X509)
  {
    connect_data.client_certificate = data->auth.x509.cert;
    connect_data.client_private_key = data->auth.x509.key;
    connect_data.password = AZ_SPAN_EMPTY;
  }
  else
  {
    // HFSM_TODO: not implemented.
    az_platform_critical_error();
  }

  size_t buffer_size;

  az_hfsm_pipeline_error_handler(
      (az_hfsm_policy*)me,
      az_iot_provisioning_client_get_client_id(
          me->_internal.provisioning_client,
          (char*)az_span_ptr(data->client_id_buffer),
          (size_t)az_span_size(data->client_id_buffer),
          &buffer_size));
  connect_data.client_id = az_span_slice(data->client_id_buffer, 0, (int32_t)buffer_size);

  az_hfsm_pipeline_error_handler(
      (az_hfsm_policy*)me,
      az_iot_provisioning_client_get_user_name(
          me->_internal.provisioning_client,
          (char*)az_span_ptr(data->username_buffer),
          (size_t)az_span_size(data->username_buffer),
          &buffer_size));
  connect_data.username = az_span_slice(data->username_buffer, 0, (int32_t)buffer_size);

  az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_CONNECT_REQ, .data = &connect_data });
}

static az_hfsm_return_type idle(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_hfsm_iot_provisioning_policy* this_policy = (az_hfsm_iot_provisioning_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
    case AZ_IOT_PROVISIONING_STOP:
      // No-op.
      break;

    case AZ_IOT_PROVISIONING_REGISTER_REQ:
      az_hfsm_transition_peer(me, idle, started);
      az_hfsm_transition_substate(me, started, connecting);
      _dps_connect(this_policy, (az_hfsm_iot_provisioning_register_data*)event.data);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE void _dps_disconnect(az_hfsm_iot_provisioning_policy* me)
{
  az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ, .data = NULL });
}

static az_hfsm_return_type started(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_hfsm_iot_provisioning_policy* this_policy = (az_hfsm_iot_provisioning_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_IOT_PROVISIONING_STOP:
      az_hfsm_transition_substate(me, started, disconnecting);
      _dps_disconnect(this_policy);
      break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
      az_hfsm_transition_peer(me, started, idle);
      az_hfsm_send_event((az_hfsm*)((az_hfsm_policy*)this_policy)->outbound, event);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE void _dps_subscribe(az_hfsm_iot_provisioning_policy* me)
{
  // HFSM_TODO: This should be initialized by the MQTT stack and a reference held until subscribe
  // completes. AzRTOS may require pointers from an NX TCB.
  az_hfsm_mqtt_sub_data data = (az_hfsm_mqtt_sub_data){
    .topic_filter = AZ_SPAN_LITERAL_FROM_STR(AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC),
    .qos = 1,
    .out_id = 0,
  };

  az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_SUB_REQ, &data });
}

static az_hfsm_return_type connecting(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_hfsm_iot_provisioning_policy* this_policy = (az_hfsm_iot_provisioning_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/connecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_MQTT_EVENT_CONNECT_RSP:
    {
      az_hfsm_mqtt_connack_data* data = (az_hfsm_mqtt_connack_data*)event.data;
      if (data->connack_reason == 0)
      {
        az_hfsm_transition_peer(me, connecting, connected);
        az_hfsm_transition_substate(me, connected, subscribing);
        _dps_subscribe(this_policy);
      }
      else
      {
        az_hfsm_transition_superstate(me, connecting, started);
        az_hfsm_transition_peer(me, started, idle);
        az_hfsm_send_event((az_hfsm*)((az_hfsm_policy*)this_policy)->outbound, event);
      }
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_return_type connected(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_hfsm_iot_provisioning_policy* this_policy = (az_hfsm_iot_provisioning_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/connected"));
  }

  switch (event.type)
  {
    case AZ_IOT_PROVISIONING_STOP:
      _dps_disconnect(this_policy);
      break;

    default:
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
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started//disconnecting"));
  }

  switch (event.type)
  {
    case AZ_IOT_PROVISIONING_STOP:
      // No-op.
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE void _dps_register(az_hfsm_iot_provisioning_policy* me)
{
  size_t buffer_size;

  az_hfsm_pipeline_error_handler(
      (az_hfsm_policy*)me,
      az_iot_provisioning_client_register_get_publish_topic(
          me->_internal.provisioning_client,
          az_span_ptr(me->_internal.topic_buffer),
          az_span_size(me->_internal.topic_buffer),
          &buffer_size));

  // HFSM_TODO: Payload

  az_hfsm_mqtt_pub_data data = (az_hfsm_mqtt_pub_data){
    .topic = az_span_slice(me->_internal.topic_buffer, 0, (int32_t)buffer_size),
    .payload = AZ_SPAN_EMPTY,
    .qos = 1,
    .out_id = 0,
  };

  az_hfsm_send_event(
      (az_hfsm*)((az_hfsm_policy*)me)->outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_PUB_REQ, .data = &data });
}

static az_hfsm_return_type subscribing(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_hfsm_iot_provisioning_policy* this_policy = (az_hfsm_iot_provisioning_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(
        event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/connected/subscribing"));
  }

  switch (event.type)
  {
    case AZ_HFSM_MQTT_EVENT_SUBACK_RSP:
      az_hfsm_transition_peer(me, subscribing, subscribed);
      az_hfsm_transition_substate(me, subscribed, start_register);
      _dps_register(this_policy);
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
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/connected/subscribed"));
  }

  switch (event.type)
  {
    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_return_type start_register(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(
        event.type,
        AZ_SPAN_FROM_STR("az_iot_provisioning/started/connected/subscribed/start_register"));
  }

  switch (event.type)
  {
    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
      az_hfsm_transition_peer(me, start_register, wait_register);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_return_type wait_register(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(
        event.type,
        AZ_SPAN_FROM_STR("az_iot_provisioning/started/connected/subscribed/wait_register"));
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

static az_hfsm_return_type delay(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(
        event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/connected/subscribed/delay"));
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

static az_hfsm_return_type query(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(
        event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/connected/subscribed/query"));
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
