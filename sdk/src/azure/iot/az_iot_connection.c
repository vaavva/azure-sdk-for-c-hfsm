// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/iot/az_iot_connection.h>

#include <azure/core/_az_cfg.h>

static az_result root(az_hfsm* me, az_event event);

static az_result idle(az_hfsm* me, az_event event);
static az_result started(az_hfsm* me, az_event event);
static az_result faulted(az_hfsm* me, az_event event);

static az_result connecting(az_hfsm* me, az_event event);
static az_result connected(az_hfsm* me, az_event event);
static az_result disconnecting(az_hfsm* me, az_event event);
static az_result reconnect_timeout(az_hfsm* me, az_event event);

static az_hfsm_state_handler _get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == idle || child_state == started || child_state == faulted)
  {
    parent_state = root;
  }
  else if (child_state == connecting || child_state == connected || child_state == disconnecting
      || child_state == reconnect_timeout)
  {
    parent_state = started;
  }
  else
  {
    // Unknown state.
    az_platform_critical_error();
    parent_state = NULL;
  }

  return parent_state;
}

static az_result root(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  az_hfsm_iot_provisioning_policy* this_policy = (az_hfsm_iot_provisioning_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/root"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op.
      break;

    case AZ_HFSM_EVENT_ERROR:
      if (az_result_failed(
              az_hfsm_pipeline_send_inbound_event((az_hfsm_policy*)this_policy, event)))
      {
        az_platform_critical_error();
      }
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

AZ_INLINE az_result
_dps_connect(az_hfsm_iot_provisioning_policy* me, az_hfsm_iot_provisioning_register_data* data)
{
  // Cache topic and payload buffers for later use.
  me->_internal.topic_buffer = data->topic_buffer;
  me->_internal.payload_buffer = data->payload_buffer;

  az_hfsm_mqtt_connect_data connect_data;
  connect_data.host = me->_internal.provisioning_client->_internal.global_device_endpoint;
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
      return AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }

  size_t buffer_size;

  _az_RETURN_IF_FAILED(az_iot_provisioning_client_get_client_id(
      me->_internal.provisioning_client,
      (char*)az_span_ptr(data->client_id_buffer),
      (size_t)az_span_size(data->client_id_buffer),
      &buffer_size));
  connect_data.client_id = az_span_slice(data->client_id_buffer, 0, (int32_t)buffer_size);

  _az_RETURN_IF_FAILED(az_iot_provisioning_client_get_user_name(
      me->_internal.provisioning_client,
      (char*)az_span_ptr(data->username_buffer),
      (size_t)az_span_size(data->username_buffer),
      &buffer_size));
  connect_data.username = az_span_slice(data->username_buffer, 0, (int32_t)buffer_size);

  _az_RETURN_IF_FAILED(az_hfsm_pipeline_send_outbound_event(
      (az_hfsm_policy*)me,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_CONNECT_REQ, .data = &connect_data }));

  return AZ_OK;
}

static az_result idle(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  az_hfsm_iot_provisioning_policy* this_policy = (az_hfsm_iot_provisioning_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
    case AZ_IOT_PROVISIONING_DISCONNECT_REQ:
      // No-op.
      break;

    case AZ_IOT_PROVISIONING_REGISTER_REQ:
      _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, idle, started));
      _az_RETURN_IF_FAILED(az_hfsm_transition_substate(me, started, connecting));
      _az_RETURN_IF_FAILED(
          _dps_connect(this_policy, (az_hfsm_iot_provisioning_register_data*)event.data));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _dps_disconnect(az_hfsm_iot_provisioning_policy* me)
{
  return az_hfsm_pipeline_send_outbound_event(
      (az_hfsm_policy*)me,
      (az_hfsm_event){ .type = AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ, .data = NULL });
}

static az_result started(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
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

    case AZ_IOT_PROVISIONING_DISCONNECT_REQ:
      _az_RETURN_IF_FAILED(az_hfsm_transition_substate(me, started, disconnecting));
      _az_RETURN_IF_FAILED(_dps_disconnect(this_policy));
      break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
      _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, started, idle));
      _az_RETURN_IF_FAILED(
          az_hfsm_pipeline_send_inbound_event((az_hfsm_policy*)this_policy, event));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result connecting(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  az_hfsm_iot_provisioning_policy* this_policy = (az_hfsm_iot_provisioning_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/connecting"));
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
        _az_RETURN_IF_FAILED(az_hfsm_transition_substate(me, connected, subscribing));
        _az_RETURN_IF_FAILED(_dps_subscribe(this_policy));
      }
      else
      {
        _az_RETURN_IF_FAILED(az_hfsm_transition_superstate(me, connecting, started));
        _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, started, idle));
        _az_RETURN_IF_FAILED(
            az_hfsm_pipeline_send_outbound_event((az_hfsm_policy*)this_policy, event));
      }
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result connected(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/connected"));
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

static az_result disconnecting(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/started/disconnecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
    case AZ_IOT_PROVISIONING_DISCONNECT_REQ:
      // No-op.
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}


AZ_NODISCARD az_iot_connection_options az_iot_connection_options_default()
{
    return (az_iot_connection_options){ 0 };
}

AZ_NODISCARD az_result az_iot_connection_init(
    az_iot_connection* client,
    az_context* context,
    az_mqtt* mqtt_client,
    az_credential_x509* primary_credential,
    char* client_id_buffer,
    size_t client_id_buffer_size,
    char* username_buffer,
    size_t username_buffer_size,
    char* password_buffer,
    size_t password_buffer_size,
    az_iot_connection_callback event_callback,
    az_iot_connection_options* options)
{
    return AZ_ERROR_NOT_IMPLEMENTED;
}

/// @brief Opens the connection to the broker.
/// @param client 
/// @return 
AZ_NODISCARD az_result az_iot_connection_open(az_iot_connection* client)
{
    return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_iot_connection_close(az_iot_connection* client)
{
    return AZ_ERROR_NOT_IMPLEMENTED;
}
