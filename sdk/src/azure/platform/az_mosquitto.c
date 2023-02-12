// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_mosquitto.c
 *
 * @brief Contains the az_mqtt.h interface implementation with Mosquitto MQTT
 * (https://github.com/eclipse/mosquitto).
 *
 * @remarks Both a non-blocking I/O (default) as well as a blocking I/O implementations are
 * available. To enable the blocking mode, set TRANSPORT_MQTT_SYNC.
 *
 * @note The Mosquitto Lib documentation is available at:
 * https://mosquitto.org/api/files/mosquitto-h.html
 */

#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/iot/az_iot_common.h>

#include <mosquitto.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

AZ_INLINE az_result _az_result_from_mosq(int mosquitto_ret)
{
  az_result ret;

  if (mosquitto_ret == MOSQ_ERR_SUCCESS)
  {
    ret = AZ_OK;
  }
  else
  {
    ret = _az_RESULT_MAKE_ERROR(_az_FACILITY_IOT_MQTT, mosquitto_ret);
  }

  return ret;
}

AZ_NODISCARD az_mqtt_options az_mqtt_options_default()
{
  return (az_mqtt_options){ .certificate_authority_trusted_roots = AZ_SPAN_EMPTY,
                            .implementation_specific_options = NULL };
}

AZ_NODISCARD az_result
az_mqtt_init(az_mqtt* mqtt, az_mqtt_impl* mqtt_handle, az_mqtt_options const* options)
{
  mqtt->_internal.options = options == NULL ? az_hfsm_mqtt_policy_options_default() : *options;
  mqtt->mqtt = mqtt_handler;
  mqtt->_internal._inbound_handler = NULL;

  return AZ_OK;
}

static void _az_mosqitto_on_connect(struct mosquitto* mosq, void* obj, int reason_code)
{
  az_result ret;
  az_hfsm_policy* me = (az_hfsm_policy*)obj;

  if (reason_code != 0)
  {
    /* If the connection fails for any reason, we don't want to keep on
     * retrying in this example, so disconnect. Without this, the client
     * will attempt to reconnect. */
    mosquitto_disconnect(mosq);
  }

  ret = az_hfsm_pipeline_post_inbound_event(
      me->pipeline,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_CONNECT_RSP,
                       &(az_hfsm_mqtt_connack_data){ reason_code } });

  if (az_result_failed(ret))
  {
    az_platform_critical_error();
  }
}

static void _az_mosqitto_on_disconnect(struct mosquitto* mosq, void* obj, int rc)
{
  (void)mosq;
  az_hfsm_policy* me = (az_hfsm_policy*)obj;

  az_result ret = az_hfsm_pipeline_post_inbound_event(
      me->pipeline,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP,
                       &(az_hfsm_mqtt_disconnect_data){ .disconnect_reason = rc,
                                                        .disconnect_requested = (rc == 0) } });
  if (az_result_failed(ret))
  {
    az_platform_critical_error();
  }
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
static void _az_mosqitto_on_publish(struct mosquitto* mosq, void* obj, int mid)
{
  (void)mosq;
  az_hfsm_policy* me = (az_hfsm_policy*)obj;

  az_result ret = az_hfsm_pipeline_post_inbound_event(
      me->pipeline,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_PUBACK_RSP, &(az_hfsm_mqtt_puback_data){ mid } });

  if (az_result_failed(ret))
  {
    az_platform_critical_error();
  }
}

static void _az_mosqitto_on_subscribe(
    struct mosquitto* mosq,
    void* obj,
    int mid,
    int qos_count,
    const int* granted_qos)
{
  (void)mosq;
  (void)qos_count;
  (void)granted_qos;

  az_hfsm_policy* me = (az_hfsm_policy*)obj;

  az_result ret = az_hfsm_pipeline_post_inbound_event(
      me->pipeline,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_SUBACK_RSP, &(az_hfsm_mqtt_suback_data){ mid } });

  if (az_result_failed(ret))
  {
    az_platform_critical_error();
  }
}

static void _az_mosqitto_on_unsubscribe(struct mosquitto* mosq, void* obj, int mid)
{
  (void)mosq;
  (void)obj;
  (void)mid;

  // Unsubscribe is not handled.
  az_platform_critical_error();
}

static void _az_mosquitto_on_message(
    struct mosquitto* mosq,
    void* obj,
    const struct mosquitto_message* message)
{
  (void)mosq;
  az_hfsm_policy* me = (az_hfsm_policy*)obj;

  az_result ret = az_hfsm_pipeline_post_inbound_event(
      me->pipeline,
      (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_PUB_RECV_IND,
                       &(az_hfsm_mqtt_recv_data){
                           .qos = (int8_t)message->qos,
                           .id = (int32_t)message->mid,
                           .payload = az_span_create(message->payload, message->payloadlen),
                           .topic = az_span_create_from_str(message->topic) } });

  if (az_result_failed(ret))
  {
    az_platform_critical_error();
  }
}

static void _az_mosqitto_on_log(struct mosquitto* mosq, void* obj, int level, const char* str)
{
  (void)mosq;
  (void)obj;
  (void)level;

  if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_MQTT_STACK))
  {
    // HFSM_TODO: add compiler supression macro instead of this workaround
    _az_LOG_WRITE(AZ_LOG_HFSM_MQTT_STACK, az_span_create_from_str((char*)(uintptr_t)str));
  }
}

AZ_NODISCARD az_result
az_mqtt_outbound_connect(az_mqtt* mqtt, az_mqtt_connect_data connect_data, az_context context)
{
  az_result ret;

  // IMPORTANT: application must call mosquitto_lib_init() before any Mosquitto clients are created.
  me->_internal.mqtt = mosquitto_new(
      (char*)az_span_ptr(data->client_id),
      false, // clean-session
      me);

  if (me->_internal.mqtt == NULL)
  {
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  /* Configure callbacks. This should be done before connecting ideally. */
  mosquitto_log_callback_set(me->_internal.mqtt, _az_mosqitto_on_log);
  mosquitto_connect_callback_set(me->_internal.mqtt, _az_mosqitto_on_connect);
  mosquitto_disconnect_callback_set(me->_internal.mqtt, _az_mosqitto_on_disconnect);
  mosquitto_publish_callback_set(me->_internal.mqtt, _az_mosqitto_on_publish);
  mosquitto_subscribe_callback_set(me->_internal.mqtt, _az_mosqitto_on_subscribe);
  mosquitto_unsubscribe_callback_set(me->_internal.mqtt, _az_mosqitto_on_unsubscribe);
  mosquitto_message_callback_set(me->_internal.mqtt, _az_mosquitto_on_message);

  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_tls_set(
      me->_internal.mqtt,
      (const char*)az_span_ptr(me->_internal.options.certificate_authority_trusted_roots),
      NULL,
      (const char*)az_span_ptr(data->client_certificate),
      (const char*)az_span_ptr(data->client_private_key),
      NULL))); // HFSM_TODO: Key callback for cases where the PEM files are encrypted at rest.

  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_username_pw_set(
      me->_internal.mqtt,
      (const char*)az_span_ptr(data->username),
      (const char*)az_span_ptr(data->password))));

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_connect_async(
      (struct mosquitto*)me->_internal.mqtt,
      (char*)az_span_ptr(data->host),
      data->port,
      AZ_MQTT_KEEPALIVE_SECONDS)));
#else
  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_connect(
      (struct mosquitto*)me->_internal.mqtt,
      (char*)az_span_ptr(data->host),
      data->port,
      AZ_MQTT_KEEPALIVE_SECONDS)));
#endif

  return AZ_OK;
}

AZ_INLINE az_result _az_mosquitto_disconnect(az_hfsm_mqtt_policy* me)
{
  return _az_result_from_mosq(mosquitto_disconnect((struct mosquitto*)me->_internal.mqtt));
}

AZ_INLINE az_result _az_mosquitto_pub(az_hfsm_mqtt_policy* me, az_hfsm_mqtt_pub_data* data)
{
  return _az_result_from_mosq(mosquitto_publish(
      me->_internal.mqtt,
      &data->out_id,
      (char*)az_span_ptr(data->topic), // Assumes properly formed NULL terminated string.
      az_span_size(data->payload),
      az_span_ptr(data->payload),
      data->qos,
      false));
}

AZ_INLINE az_result _az_mosquitto_sub(az_hfsm_mqtt_policy* me, az_hfsm_mqtt_sub_data* data)
{
  return _az_result_from_mosq(mosquitto_subscribe(
      (struct mosquitto*)me->_internal.mqtt,
      &data->out_id,
      (char*)az_span_ptr(data->topic_filter),
      data->qos));
}

AZ_INLINE az_result _az_mosquitto_init(az_hfsm_mqtt_policy* me)
{
#ifndef TRANSPORT_MQTT_SYNC
  struct mosquitto* mosq = (struct mosquitto*)me->_internal.mqtt;
  return _az_result_from_mosq(mosquitto_loop_start(mosq));
#else
  // No-op.
  return AZ_OK;
#endif
}

AZ_INLINE az_result _az_mosquitto_deinit(az_hfsm_mqtt_policy* me)
{
  struct mosquitto* mosq = (struct mosquitto*)me->_internal.mqtt;

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(_az_result_from_mosq(mosquitto_loop_stop(mosq, false)));
#endif

  mosquitto_destroy(mosq);

  return AZ_OK;
}

#ifdef TRANSPORT_MQTT_SYNC
AZ_INLINE az_result _az_mosquitto_process_loop(az_hfsm_mqtt_policy* me)
{
  struct mosquitto* mosq = (struct mosquitto*)me->_internal.mqtt;
  return _az_result_from_mosq(mosquitto_loop(
      mosq, AZ_MQTT_SYNC_MAX_POLLING_MILLISECONDS, AZ_IOT_COMPAT_CSDK_MAX_QUEUE_SIZE));
}
#endif

static az_result root(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  az_hfsm_mqtt_policy* this_mqtt = (az_hfsm_mqtt_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mosquitto/root"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op.
      break;

#ifdef TRANSPORT_MQTT_SYNC
    case AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP:
      ret = _az_mosquitto_process_loop(this_mqtt);
      break;
#endif

    case AZ_HFSM_EVENT_ERROR:
      if (az_result_failed(az_hfsm_pipeline_send_inbound_event((az_hfsm_policy*)this_mqtt, event)))
      {
        az_platform_critical_error();
      }
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
static az_result idle(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;

  az_hfsm_mqtt_policy* this_mqtt = (az_hfsm_mqtt_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mosquitto/root/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_HFSM_MQTT_EVENT_CONNECT_REQ:
      _az_RETURN_IF_FAILED(
          _az_mosquitto_connect(this_mqtt, (az_hfsm_mqtt_connect_data*)event.data));
      _az_RETURN_IF_FAILED(az_hfsm_transition_substate((az_hfsm*)me, idle, running));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

// Root/running
static az_result running(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;

  az_hfsm_mqtt_policy* this_mqtt = (az_hfsm_mqtt_policy*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_mosquitto/root/running"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ret = _az_mosquitto_init(this_mqtt);
      break;

    case AZ_HFSM_EVENT_EXIT:
      ret = _az_mosquitto_deinit(this_mqtt);
      break;

    case AZ_HFSM_MQTT_EVENT_PUB_REQ:
      ret = _az_mosquitto_pub(this_mqtt, (az_hfsm_mqtt_pub_data*)event.data);
      break;

    case AZ_HFSM_MQTT_EVENT_SUB_REQ:
      ret = _az_mosquitto_sub(this_mqtt, (az_hfsm_mqtt_sub_data*)event.data);
      break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ:
      ret = _az_mosquitto_disconnect(this_mqtt);
      break;

    case AZ_HFSM_MQTT_EVENT_CONNECT_RSP:
    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
    case AZ_HFSM_MQTT_EVENT_SUBACK_RSP:
    case AZ_HFSM_MQTT_EVENT_PUB_RECV_IND:
      ret = az_hfsm_pipeline_send_inbound_event((az_hfsm_policy*)me, event);
      break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
      _az_RETURN_IF_FAILED(az_hfsm_pipeline_send_inbound_event((az_hfsm_policy*)me, event));

      // HFSM_TODO: This exits the state which also destroys the MQTT object. This should not happen
      // inside of the state machine (i.e. the on_disconnect callback). Instead, the application
      // (API) should be responsible with the destroy call.
      _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, running, idle));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}
