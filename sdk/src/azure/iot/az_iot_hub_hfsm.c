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

static az_result root(az_hfsm* me, az_hfsm_event event);

static az_result idle(az_hfsm* me, az_hfsm_event event);
static az_result started(az_hfsm* me, az_hfsm_event event);

static az_result connecting(az_hfsm* me, az_hfsm_event event);
static az_result connected(az_hfsm* me, az_hfsm_event event);
static az_result disconnecting(az_hfsm* me, az_hfsm_event event);

static az_result subscribing(az_hfsm* me, az_hfsm_event event);
static az_result subscribed(az_hfsm* me, az_hfsm_event event);

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
  return (az_hfsm_iot_hub_policy_options){ .port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT };
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
  policy->_internal.sub_remaining = 0;

  _az_RETURN_IF_FAILED(az_hfsm_init((az_hfsm*)policy, root, _get_parent));
  _az_RETURN_IF_FAILED(az_hfsm_transition_substate((az_hfsm*)policy, root, idle));

  return AZ_OK;
}

static az_result root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_hfsm_iot_hub_policy* this_policy = (az_hfsm_iot_hub_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/root"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op.
      break;

    case AZ_HFSM_EVENT_ERROR:
      if (az_result_failed(
              az_hfsm_send_event((az_hfsm*)this_policy->_internal.policy.inbound, event)))
      {
        az_platform_critical_error();
      }
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

AZ_INLINE az_result _hub_connect(az_hfsm_iot_hub_policy* me, az_hfsm_iot_hub_connect_data* data)
{
  az_hfsm_mqtt_connect_data connect_data;
  connect_data.host = me->_internal.hub_client->_internal.iot_hub_hostname;
  connect_data.port = me->_internal.options.port;

  switch (data->auth_type)
  {
    case AZ_HFSM_IOT_AUTH_X509:
      connect_data.client_certificate = data->auth.x509.cert;
      connect_data.client_private_key = data->auth.x509.key;
      connect_data.password = AZ_SPAN_EMPTY;
      break;

    case AZ_HFSM_IOT_AUTH_SAS:
    default:
      // HFSM_TODO: SAS not implemented.
      az_platform_critical_error();
      break;
  }

  size_t buffer_size;

  _az_RETURN_IF_FAILED(az_iot_hub_client_get_client_id(
      me->_internal.hub_client,
      (char*)az_span_ptr(data->client_id_buffer),
      (size_t)az_span_size(data->client_id_buffer),
      &buffer_size));
  connect_data.client_id = az_span_slice(data->client_id_buffer, 0, (int32_t)buffer_size);

  _az_RETURN_IF_FAILED(az_iot_hub_client_get_user_name(
      me->_internal.hub_client,
      (char*)az_span_ptr(data->username_buffer),
      (size_t)az_span_size(data->username_buffer),
      &buffer_size));
  connect_data.username = az_span_slice(data->username_buffer, 0, (int32_t)buffer_size);

  _az_RETURN_IF_FAILED(az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_CONNECT_REQ, .data = &connect_data }));

  return AZ_OK;
}

static az_result idle(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_hfsm_iot_hub_policy* this_policy = (az_hfsm_iot_hub_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
      // No-op.
      break;

    case AZ_IOT_HUB_DISCONNECT_REQ:
    case AZ_IOT_HUB_TELEMETRY_REQ:
    case AZ_IOT_HUB_METHODS_RSP:
      ret = AZ_ERROR_HFSM_INVALID_STATE;
      break;

    case AZ_IOT_HUB_CONNECT_REQ:
      _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, idle, started));
      _az_RETURN_IF_FAILED(az_hfsm_transition_substate(me, started, connecting));
      _az_RETURN_IF_FAILED(_hub_connect(this_policy, (az_hfsm_iot_hub_connect_data*)event.data));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _hub_disconnect(az_hfsm_iot_hub_policy* me)
{
  return az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ, .data = NULL });
}

static az_result started(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_hfsm_iot_hub_policy* this_policy = (az_hfsm_iot_hub_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_IOT_HUB_DISCONNECT_REQ:
      _az_RETURN_IF_FAILED(az_hfsm_transition_substate(me, started, disconnecting));
      _az_RETURN_IF_FAILED(_hub_disconnect(this_policy));
      break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
      _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, started, idle));

      // HFSM_TODO: Disconnect data.
      _az_RETURN_IF_FAILED(az_hfsm_send_event(
          (az_hfsm*)this_policy->_internal.policy.inbound,
          (az_hfsm_event){ .type = AZ_IOT_HUB_DISCONNECT_RSP, .data = NULL }));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _hub_subscribe(az_hfsm_iot_hub_policy* me)
{
  // Methods
  az_hfsm_mqtt_sub_data data = (az_hfsm_mqtt_sub_data){
    .topic_filter = AZ_SPAN_LITERAL_FROM_STR(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC),
    .qos = 0,
    .out_id = 0,
  };

  _az_RETURN_IF_FAILED(az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_SUB_REQ, &data }));

  me->_internal.sub_remaining++;

  // C2D
  data = (az_hfsm_mqtt_sub_data){
    .topic_filter = AZ_SPAN_LITERAL_FROM_STR(AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC),
    .qos = 0,
    .out_id = 0,
  };

  _az_RETURN_IF_FAILED(az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_SUB_REQ, &data }));

  me->_internal.sub_remaining++;

  return AZ_OK;
}

static az_result connecting(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_hfsm_iot_hub_policy* this_policy = (az_hfsm_iot_hub_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/connecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_HFSM_MQTT_EVENT_CONNECT_RSP:
    {
      az_hfsm_mqtt_connack_data* data = (az_hfsm_mqtt_connack_data*)event.data;
      if (data->connack_reason == 0)
      {
        _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, connecting, connected));
        _az_RETURN_IF_FAILED(az_hfsm_send_event(
            (az_hfsm*)((az_hfsm_policy*)this_policy)->inbound,
            (az_hfsm_event){ .type = AZ_IOT_HUB_CONNECT_RSP, .data = NULL }));

        _az_RETURN_IF_FAILED(az_hfsm_transition_substate(me, connected, subscribing));
        _az_RETURN_IF_FAILED(_hub_subscribe(this_policy));
      }
      else
      {
        _az_RETURN_IF_FAILED(az_hfsm_transition_superstate(me, connecting, started));
        _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, started, idle));
        _az_RETURN_IF_FAILED(az_hfsm_send_event(
            (az_hfsm*)((az_hfsm_policy*)this_policy)->inbound,
            (az_hfsm_event){ .type = AZ_IOT_HUB_DISCONNECT_RSP, .data = NULL }));
      }
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

az_result _try_parse_methods(az_hfsm_iot_hub_policy* me, az_hfsm_mqtt_recv_data* data)
{
  az_hfsm_iot_hub_method_request_data request;
  _az_RETURN_IF_FAILED(az_iot_hub_client_methods_parse_received_topic(
      me->_internal.hub_client, data->topic, &request));

  _az_RETURN_IF_FAILED(az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.inbound,
      (az_hfsm_event){ .type = AZ_IOT_HUB_METHODS_REQ, .data = &request }));

  return AZ_OK;
}

az_result _try_parse_c2d(az_hfsm_iot_hub_policy* me, az_hfsm_mqtt_recv_data* data)
{
  az_hfsm_iot_hub_c2d_request_data request;
  _az_RETURN_IF_FAILED(az_iot_hub_client_c2d_parse_received_topic(
      me->_internal.hub_client, data->topic, &request.topic_info));

  _az_RETURN_IF_FAILED(az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.inbound,
      (az_hfsm_event){ .type = AZ_IOT_HUB_C2D_REQ, .data = &request }));

  return AZ_OK;
}

az_result _hub_message_parse(az_hfsm_iot_hub_policy* me, az_hfsm_mqtt_recv_data* data)
{
  az_result ret;
  ret == _try_parse_methods(me, data);

  if (ret == AZ_ERROR_IOT_TOPIC_NO_MATCH)
  {
    ret = _try_parse_c2d(me, data);
  }

  return ret;
}

AZ_INLINE az_result
_hub_methods_response_send(az_hfsm_iot_hub_policy* me, az_hfsm_iot_hub_method_response_data* data)
{
  size_t buffer_size;

  _az_RETURN_IF_FAILED(az_iot_hub_client_methods_response_get_publish_topic(
      me->_internal.hub_client,
      data->request_id,
      data->status,
      (char*)az_span_ptr(data->topic_buffer),
      (size_t)az_span_size(data->topic_buffer),
      &buffer_size));

  az_hfsm_mqtt_pub_data mqtt_data = (az_hfsm_mqtt_pub_data){
    .topic = az_span_slice(data->topic_buffer, 0, (int32_t)buffer_size),
    .payload = data->payload,
    .qos = 0,
    .out_id = 0,
  };

  return az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_PUB_REQ, &mqtt_data });
}

AZ_INLINE az_result
_hub_telemetry_send(az_hfsm_iot_hub_policy* me, az_hfsm_iot_hub_telemetry_data* data)
{
  size_t buffer_size;

  _az_RETURN_IF_FAILED(az_iot_hub_client_telemetry_get_publish_topic(
      me->_internal.hub_client,
      data->properties,
      (char*)az_span_ptr(data->topic_buffer),
      (size_t)az_span_size(data->topic_buffer),
      &buffer_size));

  az_hfsm_mqtt_pub_data mqtt_data = (az_hfsm_mqtt_pub_data){
    .topic = az_span_slice(data->topic_buffer, 0, (int32_t)buffer_size),
    .payload = data->data,
    .qos = 1,
    .out_id = 0,
  };

  _az_RETURN_IF_FAILED(az_hfsm_send_event(
      (az_hfsm*)me->_internal.policy.outbound,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_PUB_REQ, &mqtt_data }));

  data->out_packet_id = mqtt_data.out_id;
}

// Important regarding the type of messages received:
//  In order to receive C2D messages sent when the device was offline, clean-session must be set to
//  false. This results in subscriptions being preserved between connects. The device may start
//  receiving data from previously subscribed topics prior to getting into the subscribed state.
static az_result connected(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_hfsm_iot_hub_policy* this_policy = (az_hfsm_iot_hub_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/connected"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_IOT_HUB_TELEMETRY_REQ:
      ret = _hub_telemetry_send(this_policy, (az_hfsm_iot_hub_telemetry_data*)event.data);
      break;

    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
      // HFSM_DESIGN: Message ID matching requires storing pipelined operations and their respective
      //              types. (e.g. ID=5: Telemetry; ID=6: Methods_Response, etc).
      //              This is required to correctly send AZ_IOT_HUB_TELEMETRY_RSP,
      //              AZ_IOT_HUB_METHODS_RSP, etc

      // Pass through for upper layers to handle.
      ret = az_hfsm_send_event((az_hfsm*)this_policy->_internal.policy.inbound, event);
      break;

    case AZ_HFSM_MQTT_EVENT_PUB_RECV_IND:
      ret = _hub_message_parse(this_policy, (az_hfsm_mqtt_recv_data*)event.data);
      break;

    case AZ_IOT_HUB_METHODS_RSP:
      ret = _hub_methods_response_send(
          this_policy, (az_hfsm_iot_hub_method_response_data*)event.data);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result disconnecting(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/disconnecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
    case AZ_IOT_HUB_DISCONNECT_REQ:
      // No-op.
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result subscribing(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_hfsm_iot_hub_policy* this_policy = (az_hfsm_iot_hub_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/connected/subscribing"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // HFSM_TODO: a timer should be started to ensure the QoS0 subscriptions are received by the
      // server.
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_HFSM_MQTT_EVENT_SUBACK_RSP:
      if (--this_policy->_internal.sub_remaining <= 0)
      {
        _az_PRECONDITION(this_policy->_internal.sub_remaining >= 0);
        _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, subscribing, subscribed));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result subscribed(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  az_hfsm_iot_hub_policy* this_policy = (az_hfsm_iot_hub_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_hub/started/connected/subscribed"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}
