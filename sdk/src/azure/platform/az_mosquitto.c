// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

// Mosquitto Lib documentation is available at: https://mosquitto.org/api/files/mosquitto-h.html

#include <azure/core/az_hfsm.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>

#include <mosquitto.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type idle(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type running(az_hfsm* me, az_hfsm_event event);

static az_hfsm_state_handler _azure_mqtt_hfsm_get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == idle || child_state == running)
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

AZ_NODISCARD az_mqtt_options az_mqtt_options_default()
{
  return (az_mqtt_options){ .certificate_authority_trusted_roots = AZ_SPAN_EMPTY,
                            .client_certificate = AZ_SPAN_EMPTY,
                            .client_private_key = AZ_SPAN_EMPTY };
}

AZ_NODISCARD az_result az_mqtt_initialize(
    az_mqtt_hfsm_type* mqtt_hfsm,
    az_hfsm* parent,
    az_span host,
    int16_t port,
    az_span username,
    az_span password,
    az_span client_id,
    az_mqtt_options const* options)
{
  // HFSM_TODO: Preconditions

  mqtt_hfsm->host = host;
  mqtt_hfsm->_internal.parent = parent;
  mqtt_hfsm->port = port;
  mqtt_hfsm->username = username;
  mqtt_hfsm->password = password;
  mqtt_hfsm->client_id = client_id;

  mqtt_hfsm->_internal.options = options == NULL ? az_mqtt_options_default() : *options;

  // HFSM_DESIGN: A complex HFSM is recommended for MQTT stacks such as an external modems where the
  //              CPU may need to synchronize state with another device.
  //              For the Mosquitto implementation, a simplified 2 level, 3 state HFSM is used.

  _az_RETURN_IF_FAILED(az_hfsm_init(&mqtt_hfsm->_internal.hfsm, root, _azure_mqtt_hfsm_get_parent));
  az_hfsm_transition_substate(&mqtt_hfsm->_internal.hfsm, root, idle);

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt_pub_data_create(az_hfsm_mqtt_pub_data* data)
{
  // For Mosquitto MQTT, the stack does not perform internal allocation.
  // The application is responsible with allocating and maintaining the lifetime of the object.
  _az_PRECONDITION_NOT_NULL(data);
  // data->id can be NULL if the application doesn't want to correlate PUBACK.
  _az_PRECONDITION_VALID_SPAN(data->topic, 1, false);
  _az_PRECONDITION_VALID_SPAN(data->payload, 1, false);

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt_pub_data_destroy(az_hfsm_mqtt_pub_data* data)
{
  // For Mosquitto MQTT, the stack does not perform internal allocation.
  // The application is responsible with allocating and maintaining the lifetime of the object.
  _az_PRECONDITION_NOT_NULL(data);
  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt_sub_data_create(az_hfsm_mqtt_sub_data* data)
{
  // For Mosquitto MQTT, the stack does not perform internal allocation.
  // The application is responsible with allocating and maintaining the lifetime of the object.
  _az_PRECONDITION_NOT_NULL(data);
  // data->id can be NULL if the application doesn't want to correlate SUBACK.
  _az_PRECONDITION_VALID_SPAN(data->topic_filter, 1, false);

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt_sub_data_destroy(az_hfsm_mqtt_sub_data* data)
{
  // For Mosquitto MQTT, the stack does not perform internal allocation.
  // The application is responsible with allocating and maintaining the lifetime of the object.
  _az_PRECONDITION_NOT_NULL(data);
  return AZ_OK;
}

static void _az_mosqitto_on_connect(struct mosquitto* mosq, void* obj, int reason_code)
{
  az_mqtt_hfsm_type* me = (az_mqtt_hfsm_type*)obj;

  if (reason_code != 0)
  {
    /* If the connection fails for any reason, we don't want to keep on
     * retrying in this example, so disconnect. Without this, the client
     * will attempt to reconnect. */
    mosquitto_disconnect(mosq);
  }

  az_hfsm_send_event(
      me->_internal.parent,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_CONNECT_RSP,
                       &(az_hfsm_mqtt_connect_data){ reason_code } });
}

static void _az_mosqitto_on_disconnect(struct mosquitto* mosq, void* obj, int rc)
{
  az_mqtt_hfsm_type* me = (az_mqtt_hfsm_type*)obj;

  az_hfsm_transition_peer((az_hfsm*)me, running, idle);

  az_hfsm_send_event(
      me->_internal.parent,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP, &(az_hfsm_mqtt_disconnect_data){ rc } });
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
static void _az_mosqitto_on_publish(struct mosquitto* mosq, void* obj, int mid)
{
  az_mqtt_hfsm_type* me = (az_mqtt_hfsm_type*)obj;

  az_hfsm_send_event(
      me->_internal.parent,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_PUBACK_RSP, &(az_hfsm_mqtt_puback_data){ mid } });
}

static void _az_mosqitto_on_subscribe(
    struct mosquitto* mosq,
    void* obj,
    int mid,
    int qos_count,
    const int* granted_qos)
{
  az_mqtt_hfsm_type* me = (az_mqtt_hfsm_type*)obj;

  az_hfsm_send_event(
      me->_internal.parent,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_SUBACK_RSP, &(az_hfsm_mqtt_suback_data){ mid } });
}

static void _az_mosqitto_on_unsubscribe(struct mosquitto* mosq, void* obj, int mid)
{
  // Unsubscribe is not handled at the moment.
  az_platform_critical_error();
}

static void _az_mosquitto_on_message(
    struct mosquitto* mosq,
    void* obj,
    const struct mosquitto_message* message)
{
  az_mqtt_hfsm_type* me = (az_mqtt_hfsm_type*)obj;

  az_hfsm_send_event(
      me->_internal.parent,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_PUB_RECV_IND,
                       &(az_hfsm_mqtt_pub_data){
                           .id = &message->mid,
                           .qos = message->qos,
                           .payload = az_span_create(message->payload, message->payloadlen),
                           .topic = az_span_create_from_str(message->topic) } });
}

static void _az_mosqitto_on_log(struct mosquitto* mosq, void* obj, int level, const char* str)
{
  if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_MQTT_STACK))
  {
    _az_LOG_WRITE(AZ_LOG_HFSM_MQTT_STACK, az_span_create_from_str((char*)str));
  }
}

AZ_INLINE int _az_mosquitto_init(az_mqtt_hfsm_type* me)
{
  int rc;

  // IMPORTANT: application must call mosquitto_lib_init() before any Mosquitto clients are created.
  me->_internal.mqtt = mosquitto_new(
      az_span_ptr(me->client_id),
      false, // clean-session
      me);

  /* Configure callbacks. This should be done before connecting ideally. */
  mosquitto_log_callback_set(me->_internal.mqtt, _az_mosqitto_on_log);
  mosquitto_connect_callback_set(me->_internal.mqtt, _az_mosqitto_on_connect);
  mosquitto_disconnect_callback_set(me->_internal.mqtt, _az_mosqitto_on_disconnect);
  mosquitto_publish_callback_set(me->_internal.mqtt, _az_mosqitto_on_publish);
  mosquitto_subscribe_callback_set(me->_internal.mqtt, _az_mosqitto_on_subscribe);
  mosquitto_unsubscribe_callback_set(me->_internal.mqtt, _az_mosqitto_on_unsubscribe);
  mosquitto_message_callback_set(me->_internal.mqtt, _az_mosquitto_on_message);

  rc = mosquitto_tls_set(
      me->_internal.mqtt,
      (const char*)az_span_ptr(me->_internal.options.certificate_authority_trusted_roots),
      NULL,
      (const char*)az_span_ptr(me->_internal.options.client_certificate),
      (const char*)az_span_ptr(me->_internal.options.client_private_key),
      NULL); // HFSM_TODO: Key callback for cases where the PEM files are encrypted at rest.

  if (rc == MOSQ_ERR_SUCCESS)
  {
    rc = mosquitto_username_pw_set(
        me->_internal.mqtt,
        (const char*)az_span_ptr(me->username),
        (const char*)az_span_ptr(me->password));
  }

  return rc;
}

AZ_INLINE int _az_mosquitto_connect(az_mqtt_hfsm_type* me)
{
  return mosquitto_connect_async(
      (struct mosquitto*)me->_internal.mqtt,
      az_span_ptr(me->host),
      me->port,
      AZ_MQTT_KEEPALIVE_SECONDS);
}

AZ_INLINE int _az_mosquitto_disconnect(az_mqtt_hfsm_type* me)
{
  return mosquitto_disconnect((struct mosquitto*)me->_internal.mqtt);
}

AZ_INLINE int _az_mosquitto_pub(az_mqtt_hfsm_type* me, az_hfsm_mqtt_pub_data* data)
{
  return mosquitto_publish(
      me->_internal.mqtt,
      data->id,
      az_span_ptr(data->topic), // Assumes properly formed NULL terminated string.
      az_span_size(data->payload),
      az_span_ptr(data->payload),
      data->qos,
      false);
}

AZ_INLINE int _az_mosquitto_sub(az_mqtt_hfsm_type* me, az_hfsm_mqtt_sub_data* data)
{
  return mosquitto_subscribe(
      (struct mosquitto*)me->_internal.mqtt, data->id, az_span_ptr(data->topic_filter), data->qos);
}

AZ_INLINE int _az_mosquitto_deinit(az_mqtt_hfsm_type* me)
{
  int rc;
  rc = mosquitto_loop_stop((struct mosquitto*)me->_internal.mqtt, false);

  if (rc == MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy((struct mosquitto*)me->_internal.mqtt);
  }

  return rc;
}

AZ_INLINE void _az_mosquitto_error_adapter(az_mqtt_hfsm_type* me, int rc)
{
  if (rc != MOSQ_ERR_SUCCESS)
  {
    az_hfsm_event_data_error d = { .error_type = rc };
    az_hfsm_send_event(me->_internal.parent, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &d });
  }
}

az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;

  az_mqtt_hfsm_type* this_iot_hfsm = (az_mqtt_hfsm_type*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mosquitto/root"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op.
      break;

    case AZ_HFSM_EVENT_EXIT:
      // Exitting root state is not permitted. Flow through default:
    default:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_EXIT))
      {
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mosquitto/root: PANIC!"));
      }

      az_platform_critical_error();
      break;
  }

  return ret;
}

// Root/idle
az_hfsm_return_type idle(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;

  az_mqtt_hfsm_type* this_iot_hfsm = (az_mqtt_hfsm_type*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mosquitto/root/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      _az_mosquitto_error_adapter(this_iot_hfsm, _az_mosquitto_init(this_iot_hfsm));
      break;

    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_HFSM_MQTT_EVENT_CONNECT_REQ:
      _az_mosquitto_error_adapter(this_iot_hfsm, _az_mosquitto_connect(this_iot_hfsm));

      az_hfsm_transition_substate((az_hfsm*)me, idle, running);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

// Root/running
az_hfsm_return_type running(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;

  az_mqtt_hfsm_type* this_iot_hfsm = (az_mqtt_hfsm_type*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mosquitto/root/running"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      _az_mosquitto_error_adapter(
          this_iot_hfsm, mosquitto_loop_start(this_iot_hfsm->_internal.mqtt));
      break;

    case AZ_HFSM_EVENT_EXIT:
      // HFSM_TODO: A new event is likely required in case the loop can't stop.
      _az_mosquitto_error_adapter(this_iot_hfsm, _az_mosquitto_deinit(this_iot_hfsm));

      // IMPORTANT: application must call mosquitto_lib_cleanup()
      // after all clients have been destroyed.
      break;

    case AZ_HFSM_MQTT_EVENT_PUB_REQ:
      _az_mosquitto_error_adapter(
          this_iot_hfsm, _az_mosquitto_pub(this_iot_hfsm, (az_hfsm_mqtt_pub_data*)event.data));
      break;

    case AZ_HFSM_MQTT_EVENT_SUB_REQ:
      _az_mosquitto_error_adapter(
          this_iot_hfsm, _az_mosquitto_sub(this_iot_hfsm, (az_hfsm_mqtt_sub_data*)event.data));
      break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ:
      _az_mosquitto_disconnect(this_iot_hfsm);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}
