/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_provisioning_hfsm.c
 * @brief HFSM for Azure IoT Provisioning Operations.
 *
 * @details Implements connectivity and fault handling for an IoT Hub Client
 */

#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/iot/az_iot_provisioning_client.h>

#include <azure/core/_az_cfg.h>

static az_result root(az_event_policy* me, az_event event);

static az_result idle(az_event_policy* me, az_event event);
static az_result started(az_event_policy* me, az_event event);
static az_result faulted(az_event_policy* me, az_event event);

static az_result subscribing(az_event_policy* me, az_event event);
static az_result subscribed(az_event_policy* me, az_event event);

static az_result start_register(az_event_policy* me, az_event event);
static az_result wait_register(az_event_policy* me, az_event event);
static az_result delay(az_event_policy* me, az_event event);
static az_result query(az_event_policy* me, az_event event);

static az_event_policy_handler _get_parent(az_event_policy_handler child_state)
{
  az_event_policy_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == idle || child_state == started || child_state == faulted)
  {
    parent_state = root;
  }
  else if (child_state == subscribing || child_state == subscribed)
  {
    parent_state = started;
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

static az_result root(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_iot_provisioning_client* client = (az_iot_provisioning_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_MQTT_EVENT_CONNECT_REQ:
    case AZ_MQTT_EVENT_CONNECT_RSP:
    case AZ_MQTT_EVENT_DISCONNECT_REQ:
    case AZ_MQTT_EVENT_DISCONNECT_RSP:
      // No-op.
      break;

    case AZ_HFSM_EVENT_ERROR:
      if (az_result_failed(az_event_policy_send_inbound_event(me, event)))
      {
        az_platform_critical_error();
      }

      _az_RETURN_IF_FAILED(_az_hfsm_transition_substate((_az_hfsm*)me, root, faulted));
      break;

    case AZ_HFSM_EVENT_EXIT:
    default:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_EXIT))
      {
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_iot_provisioning: PANIC!"));
      }

      az_platform_critical_error();
      break;
  }

  return ret;
}

static az_result faulted(az_event_policy* me, az_event event)
{
  az_result ret = AZ_ERROR_HFSM_INVALID_STATE;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/faulted"));
  }

  return ret;
}

static az_result started(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_iot_provisioning_client* client = (az_iot_provisioning_client*)me;

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

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _dps_subscribe(az_iot_provisioning_client* me)
{
  az_mqtt_sub_data data = (az_mqtt_sub_data){
    .topic_filter = AZ_SPAN_LITERAL_FROM_STR(AZ_IOT_PROVISIONING_CLIENT_REGISTER_SUBSCRIBE_TOPIC),
    .qos = 1,
    .out_id = 0,
  };

  return az_event_policy_send_outbound_event(
      (az_event_policy*)me, (az_event){ .type = AZ_MQTT_EVENT_SUB_REQ, &data });
}

static az_result idle(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_iot_provisioning_client* client = (az_iot_provisioning_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_IOT_PROVISIONING_REGISTER_REQ:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, idle, started));
      _az_RETURN_IF_FAILED(_az_hfsm_transition_substate((_az_hfsm*)me, started, subscribing));
      _az_RETURN_IF_FAILED(_dps_subscribe(client));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _dps_register(az_iot_provisioning_client* me)
{
  size_t buffer_size;

  _az_RETURN_IF_FAILED(az_iot_provisioning_client_register_get_publish_topic(
      me,
      (char*)az_span_ptr(me->_internal.register_data->topic_buffer),
      (size_t)az_span_size(me->_internal.register_data->topic_buffer),
      &buffer_size));

  // HFSM_TODO: Payload

  az_mqtt_pub_data data = (az_mqtt_pub_data){
    .topic = az_span_slice(me->_internal.register_data->topic_buffer, 0, (int32_t)buffer_size),
    .payload = AZ_SPAN_EMPTY,
    .qos = 1,
    .out_id = 0,
  };

  _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(
      (az_event_policy*)me, (az_event){ .type = AZ_MQTT_EVENT_PUB_REQ, .data = &data }));

  return AZ_OK;
}

static az_result subscribing(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_iot_provisioning_client* client = (az_iot_provisioning_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/subscribing"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_MQTT_EVENT_SUBACK_RSP:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, subscribing, subscribed));
      _az_RETURN_IF_FAILED(_az_hfsm_transition_substate((_az_hfsm*)me, subscribed, start_register));
      _az_RETURN_IF_FAILED(_dps_register(client));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result subscribed(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/subscribed"));
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

static az_result start_register(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(
        event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/subscribed/start_register"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_MQTT_EVENT_PUBACK_RSP:
      _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, start_register, wait_register));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _dps_is_registration_complete(
    az_iot_provisioning_client* me,
    az_iot_provisioning_client_register_response* register_response,
    az_mqtt_pub_data* data,
    bool* out_complete)
{
  // HFSM_DESIGN: gracefully ignore topic that is not meant for this subclient.
  az_result ret = az_iot_provisioning_client_parse_received_topic_and_payload(
      me, data->topic, data->payload, register_response);

  if (az_result_succeeded(ret) || ret == AZ_ERROR_IOT_TOPIC_NO_MATCH)
  {
    *out_complete
        = az_iot_provisioning_client_operation_complete(register_response->operation_status);

    if (!(*out_complete))
    {
      // Cache pending operation information require for next actions.
      me->_internal.register_data->_internal.operation_id = az_span_copy(
          me->_internal.register_data->operation_id_buffer, register_response->operation_id);
      me->_internal.register_data->_internal.retry_after_seconds
          = register_response->retry_after_seconds;
    }

    ret = AZ_OK;
  }

  return ret;
}

static az_result wait_register(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_iot_provisioning_client* client = (az_iot_provisioning_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(
        event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/subscribed/wait_register"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_MQTT_EVENT_PUB_RECV_IND:
    {
      bool provisioning_complete;
      az_iot_provisioning_client_register_response response;

      _az_RETURN_IF_FAILED(_dps_is_registration_complete(
          client, &response, (az_mqtt_pub_data*)event.data, &provisioning_complete));

      if (provisioning_complete)
      {
        _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event(
            (az_event_policy*)client,
            (az_event){ .type = AZ_IOT_PROVISIONING_REGISTER_RSP, .data = &response }));

        _az_RETURN_IF_FAILED(_az_iot_connection_api_callback(
            client->_internal.connection,
            (az_event){ .type = AZ_IOT_PROVISIONING_REGISTER_RSP, .data = &response }));
      }
      else
      {
        _az_RETURN_IF_FAILED(az_event_policy_send_inbound_event(
            (az_event_policy*)client,
            (az_event){ .type = AZ_IOT_PROVISIONING_REGISTER_IND, .data = &response }));

        _az_RETURN_IF_FAILED(_az_iot_connection_api_callback(
            client->_internal.connection,
            (az_event){ .type = AZ_IOT_PROVISIONING_REGISTER_IND, .data = &response }));

        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, wait_register, delay));
      }
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _dps_send_query(az_iot_provisioning_client* me)
{
  size_t buffer_size;

  _az_RETURN_IF_FAILED(az_iot_provisioning_client_query_status_get_publish_topic(
      me,
      me->_internal.register_data->_internal.operation_id,
      (char*)az_span_ptr(me->_internal.register_data->topic_buffer),
      (size_t)az_span_size(me->_internal.register_data->topic_buffer),
      &buffer_size));

  az_mqtt_pub_data data = (az_mqtt_pub_data){
    .topic = az_span_slice(me->_internal.register_data->topic_buffer, 0, (int32_t)buffer_size),
    .payload = AZ_SPAN_EMPTY,
    .qos = 1,
    .out_id = 0
  };

  return az_event_policy_send_outbound_event(
      (az_event_policy*)me, (az_event){ .type = AZ_MQTT_EVENT_PUB_REQ, .data = &data });
}

AZ_INLINE az_result _dps_start_timer(az_iot_provisioning_client* me)
{
  _az_event_pipeline* pipeline = &me->_internal.connection->_internal.event_pipeline;
  _az_event_pipeline_timer* timer = &me->_internal.register_data->_internal.register_timer;

  _az_RETURN_IF_FAILED(_az_event_pipeline_timer_create(pipeline, timer));

  int32_t delay_milliseconds
      = (int32_t)me->_internal.register_data->_internal.retry_after_seconds * 1000;
  if (delay_milliseconds <= 0)
  {
    delay_milliseconds = AZ_IOT_PROVISIONING_RETRY_MINIMUM_TIMEOUT_SECONDS * 1000;
  }

  _az_RETURN_IF_FAILED(az_platform_timer_start(&timer->platform_timer, delay_milliseconds));
}

AZ_INLINE az_result _dps_stop_timer(az_iot_provisioning_client* me)
{
  _az_event_pipeline_timer* timer = &me->_internal.register_data->_internal.register_timer;
  return az_platform_timer_destroy(&timer->platform_timer);
}

static az_result delay(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_iot_provisioning_client* client = (az_iot_provisioning_client*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/subscribed/delay"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      _dps_start_timer(client);
      break;

    case AZ_HFSM_EVENT_EXIT:
      _dps_stop_timer(client);
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      if (event.data == &client->_internal.register_data->_internal.register_timer)
      {
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, delay, query));
        _az_RETURN_IF_FAILED(_dps_send_query(client));
      }
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result query(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/subscribed/query"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_MQTT_EVENT_PUBACK_RSP:
      ret = _az_hfsm_transition_peer((_az_hfsm*)me, query, wait_register);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_NODISCARD az_result _az_iot_provisioning_subclient_init(
    _az_hfsm* hfsm,
    _az_iot_subclient* subclient,
    az_iot_connection* connection)
{
  // Policy will be configured by injecting the HFSM into the subclient.
  _az_RETURN_IF_FAILED(_az_hfsm_init(hfsm, root, _get_parent, NULL, NULL));
  _az_RETURN_IF_FAILED(_az_hfsm_transition_substate(hfsm, root, idle));

  subclient->policy = (az_event_policy*)hfsm;
  _az_RETURN_IF_FAILED(
      _az_iot_subclients_policy_add_client(&connection->_internal.subclient_policy, subclient));
}

AZ_NODISCARD az_result az_iot_provisioning_client_register(
    az_iot_provisioning_client* client,
    az_context* context,
    az_iot_provisioning_register_data* data)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  _az_PRECONDITION_VALID_SPAN(data->operation_id_buffer, 1, false);
  _az_PRECONDITION_VALID_SPAN(data->topic_buffer, 1, false);
  _az_PRECONDITION_VALID_SPAN(data->payload_buffer, 1, false);

  data->_internal.retry_after_seconds = 0;
  data->_internal.operation_id = AZ_SPAN_EMPTY;
  data->_internal.context = context;

  return _az_event_pipeline_post_outbound_event(
      &client->_internal.connection->_internal.event_pipeline,
      (az_event){ .type = AZ_IOT_PROVISIONING_REGISTER_REQ, .data = data });
}
