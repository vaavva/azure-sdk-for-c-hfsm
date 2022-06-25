// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_span.h>
#include <azure/core/az_hfsm.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_mqtt.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>

#include <stdio.h>
#include <stdlib.h>
#include <mosquitto.h>

#include <azure/core/_az_cfg.h>

static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event);

static az_hfsm_state_handler azure_mqtt_hfsm_get_parent(az_hfsm_state_handler child_state)
{
  return NULL;
}

AZ_NODISCARD az_mqtt_options az_mqtt_options_default()
{
  return (az_mqtt_options) { .certificate_authority_trusted_roots = AZ_SPAN_EMPTY,
                             .client_certificate = AZ_SPAN_EMPTY,
                             .client_private_key = AZ_SPAN_EMPTY };
}

AZ_NODISCARD az_result az_mqtt_initialize(
  az_mqtt_hfsm_type* mqtt_hfsm,
  az_hfsm_dispatch* iot_client,
  az_span host,
  int16_t port,
  az_span username,
  az_span password,
  az_span client_id,
  az_mqtt_options const* options
  )
{
  // CPOP_TODO: Preconditions

  mqtt_hfsm->host = host;
  mqtt_hfsm->_internal.iot_client = iot_client;
  mqtt_hfsm->port = port;
  mqtt_hfsm->username = username;
  mqtt_hfsm->password = password;
  mqtt_hfsm->client_id = client_id;
  
  mqtt_hfsm->_internal.options = options == NULL ? az_mqtt_options_default() : *options;

  // HFSM_DESIGN: AZ_HFSM is good to have for simpler MQTT stacks such as an external modems.
  //              For the Mosquitto implementation, this is used only to convert from parameters to
  //              events.

  return az_hfsm_init(&mqtt_hfsm->_internal.hfsm, root, azure_mqtt_hfsm_get_parent);
}

static void _az_mosqitto_on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
  az_mqtt_hfsm_type* me = (az_mqtt_hfsm_type*)obj;

	/* Print out the connection result. mosquitto_connack_string() produces an
	 * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
	 * clients is mosquitto_reason_string().
	 */
	printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
	if(reason_code != 0){
		/* If the connection fails for any reason, we don't want to keep on
		 * retrying in this example, so disconnect. Without this, the client
		 * will attempt to reconnect. */
		mosquitto_disconnect(mosq);
	}

	/* You may wish to set a flag here to indicate to your application that the
	 * client is now connected. */
}

static void _az_mosqitto_on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
	printf("MOSQ: DISCONNECT reason=%d\n", rc);
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
static void _az_mosqitto_on_publish(struct mosquitto *mosq, void *obj, int mid)
{
	printf("MOSQ: Message with mid %d has been published.\n", mid);
}

static void _az_mosqitto_on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
	printf("MOSQ: Subscribed with mid %d; %d topics.\n", mid, qos_count);
	for(int i = 0; i < qos_count; i++)
	{
		printf("MOSQ: \t QoS %d\n", granted_qos[i]);
	}
}

static void _az_mosqitto_on_unsubscribe(struct mosquitto *mosq, void *obj, int mid)
{
	printf("MOSQ: Unsubscribing using message with mid %d.\n", mid);
}

static void _az_mosqitto_on_log(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	char* log_level;

	switch (level)
	{
		case MOSQ_LOG_INFO:
			log_level = "INFO";
			break;
		case MOSQ_LOG_NOTICE:
			log_level = "NOTI";
			break;
		case MOSQ_LOG_WARNING:
			log_level = "WARN";
			break;
		case MOSQ_LOG_ERR:
			log_level = "ERR ";
			break;
		case MOSQ_LOG_DEBUG:
			log_level = "DBUG";
			break;
		case MOSQ_LOG_SUBSCRIBE:
			log_level = "SUB ";
			break;
		case MOSQ_LOG_UNSUBSCRIBE:
			log_level = "USUB";
			break;
		case MOSQ_LOG_WEBSOCKETS:
			log_level = "WSCK";
			break;
		default:
			log_level = "UNKN";
	}

  // HFSM_TODO: Temporary direct use of print. All messages will be routed through az_log, under  
  //            dedicated AZ_LOG_MQTT_* classifications.
	printf("MOSQ [%s] %s\n", log_level, str);
}

AZ_INLINE int _az_mosquitto_init(az_mqtt_hfsm_type* me)
{
  int rc;

  // IMPORTANT: application must call mosquitto_lib_init() before any Mosquitto clients are created.
  me->_internal.mqtt = mosquitto_new(
    az_span_ptr(me->client_id), 
    false,  //clean-session 
    me);

	/* Configure callbacks. This should be done before connecting ideally. */
	mosquitto_log_callback_set(me->_internal.mqtt, _az_mosqitto_on_log);

	mosquitto_connect_callback_set(me->_internal.mqtt, _az_mosqitto_on_connect);
	mosquitto_disconnect_callback_set(me->_internal.mqtt, _az_mosqitto_on_disconnect);
	mosquitto_publish_callback_set(me->_internal.mqtt, _az_mosqitto_on_publish);
	mosquitto_subscribe_callback_set(me->_internal.mqtt, _az_mosqitto_on_subscribe);
	mosquitto_unsubscribe_callback_set(me->_internal.mqtt, _az_mosqitto_on_unsubscribe);
	
	rc = mosquitto_tls_set(
		me->_internal.mqtt,
		(const char*)az_span_ptr(me->_internal.options.certificate_authority_trusted_roots),
		NULL,
		(const char*)az_span_ptr(me->_internal.options.client_certificate),
		(const char*)az_span_ptr(me->_internal.options.client_private_key),
		NULL);  // HFSM_TODO: Key callback for cases where the PEM files are encrypted at rest.

	if (rc == MOSQ_ERR_SUCCESS)
	{
    rc = mosquitto_username_pw_set(
      me->_internal.mqtt, 
      (const char*) az_span_ptr(me->username), 
      (const char*) az_span_ptr(me->password));
  }

	if (rc == MOSQ_ERR_SUCCESS)
	{
    rc = mosquitto_loop_start(me->_internal.mqtt);
  }

  return rc;
}

AZ_INLINE int _az_mosquitto_connect(az_mqtt_hfsm_type* me)
{
  return mosquitto_connect_async(
    me->_internal.mqtt, 
    az_span_ptr(me->host), 
    me->port, 
    AZ_MQTT_KEEPALIVE_SECONDS);
}

AZ_INLINE int _az_mosquitto_pub(az_mqtt_hfsm_type* me, az_span topic, az_span payload, int8_t qos)
{
  // HFSM_TODO: mid
  return mosquitto_publish(
    me->_internal.mqtt, 
    NULL, 
    az_span_ptr(topic), 
    az_span_size(payload), 
    az_span_ptr(payload), 
    qos, 
    false);
}

AZ_INLINE int _az_mosquitto_sub(az_mqtt_hfsm_type* me, az_span topic, int8_t qos)
{
    // HFSM_TODO
}

AZ_INLINE int _az_mosquitto_deinit(az_mqtt_hfsm_type* me)
{
      mosquitto_loop_stop(me->_internal.mqtt);
      mosquitto_destroy(me->_internal.mqtt);

}

AZ_INLINE void _az_mosquitto_error_adapter(az_mqtt_hfsm_type* me, int rc)
{
  if (rc != MOSQ_ERR_SUCCESS)
	{
    az_hfsm_event_data_error d = {.error_type = rc};
    az_hfsm_send_event(me, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &d });
	}
}

az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;

  az_mqtt_hfsm_type* this_iot_hfsm = (az_mqtt_hfsm_type*)me;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ENTRY))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_ENTRY, AZ_SPAN_FROM_STR("az_mosquitto/root"));
      }

      _az_mosquitto_error_adapter(
        this_iot_hfsm, 
        _az_mosquitto_init(this_iot_hfsm));
      break;
    
    case AZ_HFSM_MQTT_EVENT_CONNECT_REQ:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_MQTT_EVENT_CONNECT_REQ))
      {
        _az_LOG_WRITE(AZ_HFSM_MQTT_EVENT_CONNECT_REQ, AZ_SPAN_FROM_STR("az_mosquitto/root"));
      }

      _az_mosquitto_connect(this_iot_hfsm);
      break;

    case AZ_HFSM_MQTT_EVENT_PUB_REQ:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_MQTT_EVENT_PUB_REQ))
      {
        _az_LOG_WRITE(AZ_HFSM_MQTT_EVENT_PUB_REQ, AZ_SPAN_FROM_STR("az_mosquitto/root"));
      }

      //_az_mosquitto_pub(this_iot_hfsm, topic, data);
      break;

    case AZ_HFSM_MQTT_EVENT_SUB_REQ:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_MQTT_EVENT_SUB_REQ))
      {
        _az_LOG_WRITE(AZ_HFSM_MQTT_EVENT_SUB_REQ, AZ_SPAN_FROM_STR("az_mosquitto/root"));
      }

      //_az_mosquitto_pub(this_iot_hfsm, topic, qos);
      break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ))
      {
        _az_LOG_WRITE(AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ, AZ_SPAN_FROM_STR("az_mosquitto/root"));
      }

      _az_mosquitto_disconnect(this_iot_hfsm);
      break;

    case AZ_HFSM_EVENT_EXIT:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_EXIT))
      {
        _az_LOG_WRITE(AZ_LOG_HFSM_EXIT, AZ_SPAN_FROM_STR("az_mosquitto/root"));
      }

      _az_mosquitto_deinit(this_iot_hfsm);
      // IMPORTANT: application must call mosquitto_lib_cleanup() 
      // after all clients have been destroyed.

      break;

    default:
      if (_az_LOG_SHOULD_WRITE(AZ_LOG_HFSM_ERROR))
      {
        //HFSM_TODO: Logging should print out error codes.
        _az_LOG_WRITE(AZ_LOG_HFSM_ERROR, AZ_SPAN_FROM_STR("az_mosquitto/root"));
      }

      az_platform_critical_error();
      break;
  }

  return ret;
}
