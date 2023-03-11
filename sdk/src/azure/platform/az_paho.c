// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_paho.c
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
// HFSM_TODO: Following include for AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS only.
#include <azure/iot/az_iot_common.h>

#include <stdlib.h>
#ifdef _MSC_VER
#pragma warning(push)
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <azure/core/_az_cfg.h>

AZ_INLINE az_result _az_result_from_paho(int paho_ret)
{
  az_result ret;

  if (paho_ret == MQTTCLIENT_SUCCESS)
  {
    ret = AZ_OK;
  }
  else
  {
    ret = _az_RESULT_MAKE_ERROR(_az_FACILITY_IOT_MQTT, paho_ret);
  }

  return ret;
}

AZ_NODISCARD az_mqtt_options az_mqtt_options_default()
{
  return (az_mqtt_options){ .platform_options.certificate_authority_trusted_roots = NULL };
}

AZ_NODISCARD az_result az_mqtt_init(az_mqtt* mqtt, az_mqtt_options const* options)
{
  _az_PRECONDITION_NOT_NULL(mqtt);
  mqtt->_internal.options = options == NULL ? az_mqtt_options_default() : *options;
}

AZ_NODISCARD az_result
az_mqtt_outbound_connect(az_mqtt* mqtt, az_context* context, az_mqtt_connect_data* connect_data)
{
  if (!mqtt->paho_handle)
  {
    // Create the Paho MQTT client.

    _az_RETURN_IF_FAILED(_az_result_from_paho(MQTTClient_create(
        &mqtt->paho_handle,
        az_span_ptr(connect_data->host),
        az_span_ptr(connect_data->client_id),
        MQTTCLIENT_PERSISTENCE_NONE,
        NULL)));
  }

  // Set MQTT connection options.
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;
  mqtt_connect_options.username = az_span_ptr(connect_data->username);
  mqtt_connect_options.password = az_span_ptr(connect_data->password);
  mqtt_connect_options.cleansession = false; // Set to false so can receive any pending messages.
  mqtt_connect_options.keepAliveInterval = AZ_IOT_DEFAULT_MQTT_CONNECT_KEEPALIVE_SECONDS;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  mqtt_ssl_options.verify = 1;
  mqtt_ssl_options.enableServerCertAuth = 1;
  mqtt_ssl_options.keyStore = az_span_ptr(connect_data->certificate.key);
  if (az_span_size(mqtt->_internal.options.platform_options.certificate_authority_trusted_roots)
      != 0) // Is only set if required by OS.
  {
    mqtt_ssl_options.trustStore
        = (char*)az_span_ptr(mqtt->_internal.options.platform_options.certificate_authority_trusted_roots);
  }
  mqtt_connect_options.ssl = &mqtt_ssl_options;

  // Connect MQTT client to the Azure IoT Device Provisioning Service.
  int rc = MQTTClient_connect(mqtt->paho_handle, &mqtt_connect_options);

  if (rc >= 0)
  {
    _az_RETURN_IF_FAILED(az_mqtt_inbound_connack(
        mqtt, &(az_mqtt_connack_data){ .connack_reason = rc, .tls_authentication_error = false }));
  }
  else
  {
    switch (rc)
    {
      case MQTTCLIENT_DISCONNECTED:
      case MQTTCLIENT_FAILURE:
      {
        // Paho is not specific regarding TLS auth error.
        _az_RETURN_IF_FAILED(az_mqtt_inbound_disconnect(
            mqtt,
            &(az_mqtt_disconnect_data){ .disconnect_requested = false,
                                        .tls_authentication_error = false }));
        break;
      }

      default:
        return AZ_ERROR_NOT_IMPLEMENTED;
        break;
    }
  }

  return _az_result_from_paho(rc);
}

AZ_NODISCARD az_result
az_mqtt_outbound_sub(az_mqtt* mqtt, az_context* context, az_mqtt_sub_data* sub_data)
{
  // MQTT Packet ID is not supported by Paho.
  sub_data->out_id = 0;
  int rc = MQTTClient_subscribe(
      mqtt->paho_handle, az_span_ptr(sub_data->topic_filter), sub_data->qos);

  switch (rc)
  {
    case MQTTCLIENT_SUCCESS:
      _az_RETURN_IF_FAILED(az_mqtt_inbound_suback(mqtt, &(az_mqtt_suback_data){ .id = 0 }));
      break;

    case MQTTCLIENT_DISCONNECTED:
    {
      // Paho is not specific regarding TLS auth error.
      _az_RETURN_IF_FAILED(az_mqtt_inbound_disconnect(
          mqtt,
          &(az_mqtt_disconnect_data){ .disconnect_requested = false,
                                      .tls_authentication_error = false }));
      break;
    }

    default:
      return AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }

  return _az_result_from_paho(rc);
}

AZ_NODISCARD az_result
az_mqtt_outbound_pub(az_mqtt* mqtt, az_context* context, az_mqtt_pub_data* pub_data)
{
  // Set MQTT message options.
  MQTTClient_message pubmsg = MQTTClient_message_initializer;
  pubmsg.payload = az_span_ptr(pub_data->payload);
  pubmsg.payloadlen = az_span_size(pub_data->payload);
  pubmsg.qos = pub_data->qos;
  pubmsg.retained = 0;

  // Publish the register request.
  int rc = MQTTClient_publishMessage(
      mqtt->paho_handle, az_span_ptr(pub_data->topic), &pubmsg, NULL);

  switch (rc)
  {
    case MQTTCLIENT_SUCCESS:
      pub_data->out_id = pubmsg.msgid;
      _az_RETURN_IF_FAILED(
          az_mqtt_inbound_puback(mqtt, &(az_mqtt_puback_data){ .id = pub_data->out_id }));
      break;

    case MQTTCLIENT_DISCONNECTED:
    {
      // Paho is not specific regarding TLS auth error.
      _az_RETURN_IF_FAILED(az_mqtt_inbound_disconnect(
          mqtt,
          &(az_mqtt_disconnect_data){ .disconnect_requested = false,
                                      .tls_authentication_error = false }));
      break;
    }

    default:
      return AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }
}

AZ_NODISCARD az_result az_mqtt_outbound_disconnect(az_mqtt* mqtt, az_context* context)
{
  int rc = MQTTClient_disconnect(mqtt->paho_handle, AZ_IOT_MQTT_DISCONNECT_MS);
  switch (rc)
  {
    case MQTTCLIENT_SUCCESS:
      _az_RETURN_IF_FAILED(az_mqtt_inbound_disconnect(
          mqtt,
          &(az_mqtt_disconnect_data){ .disconnect_requested = true,
                                      .tls_authentication_error = false }));
      break;

    default:
      return AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }
}

AZ_NODISCARD az_result az_mqtt_wait_for_event(az_mqtt* mqtt, int32_t timeout)
{
  // Release previous objects.
  if (mqtt->last_topic)
  {
    MQTTClient_free(mqtt->last_topic);
    mqtt->last_topic = NULL;
  }

  if (mqtt->last_message)
  {
    MQTTClient_freeMessage(&mqtt->last_message);
    mqtt->last_message = NULL;
  }

  int rc;
  int topic_len = 0;
  char* topic = NULL;
  MQTTClient_message* message = NULL;

  rc = MQTTClient_receive(mqtt->paho_handle, &topic, &topic_len, &message, timeout);

  switch (rc)
  {
    case MQTTCLIENT_SUCCESS:
    {
      mqtt->last_topic = topic;
      mqtt->last_message = message;

      az_mqtt_recv_data data = (az_mqtt_recv_data){
        .id = message->msgid,
        .payload = az_span_create(message->payload, message->payloadlen),
        .qos = message->qos,
        .topic = az_span_create(topic, topic_len),
      };

      _az_RETURN_IF_FAILED(az_mqtt_inbound_recv(mqtt, &data));
      break;
    }

    case MQTTCLIENT_DISCONNECTED:
    {
      _az_RETURN_IF_FAILED(az_mqtt_inbound_disconnect(
          mqtt,
          &(az_mqtt_disconnect_data){ .disconnect_requested = false,
                                      .tls_authentication_error = false }));
      break;
    }

    case MQTTCLIENT_FAILURE:
    {
      if (topic == NULL)
      {
        return AZ_TIMEOUT;
      }
    }

    default:
      return AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }

  return _az_result_from_paho(rc);
}
