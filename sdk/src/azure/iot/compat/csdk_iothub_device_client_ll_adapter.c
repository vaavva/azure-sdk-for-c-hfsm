// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license
// information.

/**
 * @file csdk_iothub_device_client_ll_adapter.c
 * @brief C-SDK Compat implementation for the IoT Hub Client LL client.
 * @details C-SDK is not a non-allocating SDK. The following APIs rely on heap allocations to create
 *          objects.
 *
 */

// HFSM_TODO: This implementation is proof-of-concept only.

#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mosquitto.h"

#include <azure/az_iot.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/az_log.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/iot/compat/iothub.h>
#include <azure/iot/compat/iothub_client_options.h>
#include <azure/iot/compat/iothub_device_client_ll.h>
#include <azure/iot/compat/iothub_transport_ll.h>

#include <azure/iot/internal/az_iot_hub_hfsm.h>
#include <azure/iot/internal/az_iot_provisioning_hfsm.h>
#include <azure/iot/internal/az_iot_retry_hfsm.h>

#define SYS_PROP_MESSAGE_ID "mid"
#define SYS_PROP_MESSAGE_CREATION_TIME_UTC "ctime"
#define SYS_PROP_USER_ID "uid"
#define SYS_PROP_CORRELATION_ID "cid"
#define SYS_PROP_CONTENT_TYPE "ct"
#define SYS_PROP_CONTENT_ENCODING "ce"
#define SYS_PROP_DIAGNOSTIC_ID "diagid"
#define SYS_PROP_DIAGNOSTIC_CONTEXT "diagctx"
#define SYS_PROP_CONNECTION_DEVICE_ID "cdid"
#define SYS_PROP_CONNECTION_MODULE_ID "cmid"
#define SYS_PROP_ON "on"
#define SYS_PROP_EXP "exp"
#define SYS_PROP_TO "to"
#define SYS_COMPONENT_NAME "sub"

typedef struct IOTHUB_MESSAGE_HANDLE_DATA_TAG
{
  az_span topic;
  az_span payload;
  az_iot_message_properties properties;
} IOTHUB_MESSAGE_HANDLE_DATA;

#define Q_TYPE IOTHUB_MESSAGE_HANDLE_DATA*
#define Q_SIZE AZ_IOT_COMPAT_CSDK_MAX_QUEUE_SIZE
#include <azure/iot/compat/queue.h>

typedef struct IOTHUB_CLIENT_CORE_LL_HANDLE_DATA_TAG
{
  // HFSM for the Compat Client
  az_hfsm_policy compat_client_policy;

  bool connected;

  // Endpoint information
  az_span hub_endpoint;
  az_span device_id;
  az_span ca_path;
  az_span cert_path;
  az_span key_path;

  // Hub
  az_hfsm_pipeline hub_pipeline;
  az_hfsm_iot_hub_policy hub_policy;
  az_iot_hub_client hub_client;
  az_hfsm_mqtt_policy hub_mqtt_policy;

  char* topic_buffer;
  queue message_queue;

} IOTHUB_CLIENT_CORE_LL_HANDLE_DATA;

const TRANSPORT_PROVIDER* MQTT_Protocol(void) { return (TRANSPORT_PROVIDER*)0x4D515454; }

static void az_sdk_log_callback(az_log_classification classification, az_span message);
static bool az_sdk_log_filter_callback(az_log_classification classification);

#define LOG_COMPAT "\x1B[34mCOMPAT: \x1B[0m"
#define LOG_SDK "\x1B[33mSDK: \x1B[0m"

static const char* az_result_string(az_result result)
{
  const char* result_str;

  switch (result)
  {
    case AZ_ERROR_HFSM_INVALID_STATE:
      result_str = "AZ_ERROR_HFSM_INVALID_STATE";
      break;

    case AZ_OK:
      result_str = "AZ_OK";
      break;

    default:
      result_str = "UNKNOWN";
  }

  return result_str;
}

static void az_sdk_log_callback(az_log_classification classification, az_span message)
{
  const char* class_str;

  switch (classification)
  {
    case AZ_HFSM_EVENT_ENTRY:
      class_str = "HFSM_ENTRY";
      break;
    case AZ_HFSM_EVENT_EXIT:
      class_str = "HFSM_EXIT";
      break;
    case AZ_HFSM_EVENT_TIMEOUT:
      class_str = "HFSM_TIMEOUT";
      break;
    case AZ_HFSM_EVENT_ERROR:
      class_str = "HFSM_ERROR";
      break;
    case AZ_ERROR_HFSM_INVALID_STATE:
      class_str = "HFSM_INVALID_STATE";
      break;
    case AZ_HFSM_MQTT_EVENT_CONNECT_REQ:
      class_str = "AZ_HFSM_MQTT_EVENT_CONNECT_REQ";
      break;
    case AZ_HFSM_MQTT_EVENT_CONNECT_RSP:
      class_str = "AZ_HFSM_MQTT_EVENT_CONNECT_RSP";
      break;
    case AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ:
      class_str = "AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ";
      break;
    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
      class_str = "AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP";
      break;
    case AZ_HFSM_MQTT_EVENT_PUB_RECV_IND:
      class_str = "AZ_HFSM_MQTT_EVENT_PUB_RECV_IND";
      break;
    case AZ_HFSM_MQTT_EVENT_PUB_REQ:
      class_str = "AZ_HFSM_MQTT_EVENT_PUB_REQ";
      break;
    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
      class_str = "AZ_HFSM_MQTT_EVENT_PUBACK_RSP";
      break;
    case AZ_HFSM_MQTT_EVENT_SUB_REQ:
      class_str = "AZ_HFSM_MQTT_EVENT_SUB_REQ";
      break;
    case AZ_HFSM_MQTT_EVENT_SUBACK_RSP:
      class_str = "AZ_HFSM_MQTT_EVENT_SUBACK_RSP";
      break;
    case AZ_LOG_HFSM_MQTT_STACK:
      class_str = "AZ_LOG_HFSM_MQTT_STACK";
      break;
    case AZ_LOG_MQTT_RECEIVED_TOPIC:
      class_str = "AZ_LOG_MQTT_RECEIVED_TOPIC";
      break;
    case AZ_LOG_MQTT_RECEIVED_PAYLOAD:
      class_str = "AZ_LOG_MQTT_RECEIVED_PAYLOAD";
      break;
    case AZ_IOT_PROVISIONING_REGISTER_REQ:
      class_str = "AZ_IOT_PROVISIONING_REGISTER_REQ";
      break;
    case AZ_IOT_HUB_CONNECT_REQ:
      class_str = "AZ_IOT_HUB_CONNECT_REQ";
      break;
    case AZ_IOT_HUB_CONNECT_RSP:
      class_str = "AZ_IOT_HUB_CONNECT_RSP";
      break;
    case AZ_IOT_HUB_DISCONNECT_REQ:
      class_str = "AZ_IOT_HUB_DISCONNECT_REQ";
      break;
    case AZ_IOT_HUB_DISCONNECT_RSP:
      class_str = "AZ_IOT_HUB_DISCONNECT_RSP";
      break;
    case AZ_IOT_HUB_TELEMETRY_REQ:
      class_str = "AZ_IOT_HUB_TELEMETRY_REQ";
      break;
    case AZ_IOT_HUB_METHODS_REQ:
      class_str = "AZ_IOT_HUB_METHODS_REQ";
      break;
    case AZ_IOT_HUB_METHODS_RSP:
      class_str = "AZ_IOT_HUB_METHODS_RSP";
      break;
    default:
      class_str = NULL;
  }

  if (class_str == NULL)
  {
    printf(LOG_SDK "[\x1B[31mUNKNOWN: %x\x1B[0m] %s\n", classification, az_span_ptr(message));
  }
  else if (classification == AZ_HFSM_EVENT_ERROR)
  {
    printf(LOG_SDK "[\x1B[31m%s\x1B[0m] %s\n", class_str, az_span_ptr(message));
  }
  else
  {
    printf(LOG_SDK "[\x1B[35m%s\x1B[0m] %s\n", class_str, az_span_ptr(message));
  }
}

static bool az_sdk_log_filter_callback(az_log_classification classification)
{
  (void)classification;
  // Enable all logging.
  return true;
}

void az_platform_critical_error()
{
  printf(LOG_COMPAT "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

// ***** Single-layer C-SDK Compat Layer State Machine

static az_hfsm_state_handler get_parent(az_hfsm_state_handler child_state)
{
  (void)child_state;
  return NULL;
}

static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event)
{
  IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)me;

  int32_t ret = AZ_HFSM_RETURN_HANDLED;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;

    case AZ_HFSM_EVENT_EXIT:
    case AZ_HFSM_EVENT_ERROR:
    {
      az_hfsm_event_data_error* err_data = (az_hfsm_event_data_error*)event.data;
      printf(
          LOG_COMPAT "\x1B[31mERROR\x1B[0m: az_result=%s\n",
          az_result_string(err_data->error_type));
      break;
    }

    case AZ_IOT_HUB_CONNECT_RSP:
    {
      client->connected = true;
      printf(LOG_COMPAT "HUB: Connected\n");
      // TODO: Call the connection callback.

      
    }
    break;

    case AZ_IOT_HUB_METHODS_REQ:
    {
      az_hfsm_iot_hub_method_request_data* method_req
          = (az_hfsm_iot_hub_method_request_data*)event.data;
      printf(
          LOG_COMPAT "HUB: Method received [%.*s]\n",
          az_span_size(method_req->name),
          az_span_ptr(method_req->name));

      az_hfsm_iot_hub_method_response_data method_rsp
          = (az_hfsm_iot_hub_method_response_data){ .status = 0,
                                                    .request_id = method_req->request_id,
                                                    .topic_buffer
                                                    = AZ_SPAN_FROM_BUFFER(client->topic_buffer),
                                                    .payload = AZ_SPAN_EMPTY };

      az_hfsm_send_event(
          (az_hfsm*)&client->hub_policy, (az_hfsm_event){ AZ_IOT_HUB_METHODS_RSP, &method_rsp });
    }
    break;

    case AZ_IOT_HUB_DISCONNECT_RSP:
      client->connected = false;
      printf(LOG_COMPAT "HUB: Disconnected\n");
      break;

    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
    {
      az_hfsm_mqtt_puback_data* puback = (az_hfsm_mqtt_puback_data*)event.data;
      printf(LOG_COMPAT "MQTT: PUBACK ID=%d\n", puback->id);
    }
    break;

    // Pass-through events.
    case AZ_IOT_HUB_DISCONNECT_REQ:
    case AZ_HFSM_EVENT_TIMEOUT:
      // Pass-through events.
      az_hfsm_send_event((az_hfsm*)client->compat_client_policy.outbound, event);
      break;

    default:
      printf(LOG_COMPAT "UNKNOWN event! %x\n", event.type);
      break;
  }

  return ret;
}

static az_result hub_initialize(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client)
{
  client->compat_client_policy = (az_hfsm_policy){
    .inbound = NULL,
    .outbound = (az_hfsm_policy*)&client->hub_policy,
  };

  _az_RETURN_IF_FAILED(az_hfsm_init((az_hfsm*)&client->compat_client_policy, root, get_parent));

  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &client->hub_pipeline,
      (az_hfsm_policy*)&client->compat_client_policy,
      (az_hfsm_policy*)&client->hub_mqtt_policy));

  _az_RETURN_IF_FAILED(
      az_iot_hub_client_init(&client->hub_client, client->hub_endpoint, client->device_id, NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_hub_policy_initialize(
      &client->hub_policy,
      &client->hub_pipeline,
      &client->compat_client_policy,
      (az_hfsm_policy*)&client->hub_mqtt_policy,
      &client->hub_client,
      NULL));

  // MQTT
  az_hfsm_mqtt_policy_options mqtt_options = az_hfsm_mqtt_policy_options_default();
  mqtt_options.certificate_authority_trusted_roots = client->ca_path;

  _az_RETURN_IF_FAILED(az_mqtt_initialize(
      &client->hub_mqtt_policy,
      &client->hub_pipeline,
      (az_hfsm_policy*)&client->hub_policy,
      &mqtt_options));

  return AZ_OK;
}

// ************************************* C-SDK Compat layer ************************************ //

int IoTHub_Init() { _az_RETURN_IF_FAILED(az_mqtt_init()); }

void IoTHub_Deinit()
{
  if (az_result_failed(az_mqtt_deinit()))
  {
    az_platform_critical_error();
  }
}

static void deallocate_compat_client(IOTHUB_DEVICE_CLIENT_LL_HANDLE client)
{
  if (client != NULL)
  {
    if (client->topic_buffer != NULL)
    {
      free(client->topic_buffer);
      client->topic_buffer = NULL;
    }

    if (az_span_ptr(client->hub_endpoint) != NULL)
    {
      free(az_span_ptr(client->hub_endpoint));
      client->hub_endpoint = AZ_SPAN_EMPTY;
    }

    free(client);
  }
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE IoTHubDeviceClient_LL_Create(const IOTHUB_CLIENT_CONFIG* config)
{
  _az_PRECONDITION(config->protocol == (IOTHUB_CLIENT_TRANSPORT_PROVIDER)MQTT_Protocol);

  az_result ret;
  IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client = malloc(sizeof(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA));

  if (client != NULL)
  {
    memset(client, 0, sizeof(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA));
    client->topic_buffer = malloc(AZ_IOT_MAX_TOPIC_SIZE);

    size_t hub_endpoint_buffer_size = strlen(config->iotHubName) + strlen(config->iotHubSuffix) + 2;
    client->hub_endpoint
        = az_span_create(malloc(hub_endpoint_buffer_size), hub_endpoint_buffer_size);

    if (client->topic_buffer != NULL && az_span_ptr(client->hub_endpoint) != NULL)
    {
      client->connected = false;
      client->device_id = az_span_create_from_str((char*)(uintptr_t)config->deviceId);
      az_span reminder = az_span_copy(
          client->hub_endpoint, az_span_create_from_str((char*)(uintptr_t)config->iotHubName));
      reminder = az_span_copy_u8(reminder, '.');
      reminder
          = az_span_copy(reminder, az_span_create_from_str((char*)(uintptr_t)config->iotHubSuffix));
      az_span_copy_u8(reminder, '\0');

      ret = hub_initialize(client);
    }
    else
    {
      deallocate_compat_client(client);
      client = NULL;
    }
  }

  return client;
}

void IoTHubDeviceClient_LL_Destroy(IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle)
{
  // Disconnect async (disconnect will be initiated here).
  az_result ignore = az_hfsm_pipeline_post_outbound_event(
      &iotHubClientHandle->hub_pipeline, (az_hfsm_event){ AZ_IOT_HUB_DISCONNECT_REQ, NULL });
}

IOTHUB_CLIENT_RESULT IoTHubDeviceClient_LL_SetOption(
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle,
    const char* optionName,
    const void* value)
{
  int result;

  if ((iotHubClientHandle == NULL) || (optionName == NULL) || (value == NULL))
  {
    result = IOTHUB_CLIENT_INVALID_ARG;
  }
  else if (!strcmp(optionName, OPTION_LOG_TRACE))
  {
    if (*(bool*)value == true)
    {
      az_log_set_message_callback(az_sdk_log_callback);
      az_log_set_classification_filter_callback(az_sdk_log_filter_callback);
    }
  }
  else if (!strcmp(optionName, OPTION_AUTO_URL_ENCODE_DECODE))
  {
    // HFSM_TODO: not implemented.
    result = IOTHUB_CLIENT_OK;
  }
  else if (!strcmp(optionName, OPTION_OPENSSL_ENGINE))
  {
    // HFSM_TODO: not implemented.
    result = IOTHUB_CLIENT_ERROR;
  }
  else if (!strcmp(optionName, OPTION_OPENSSL_PRIVATE_KEY_TYPE))
  {
    // HFSM_TODO: not implemented.
    result = IOTHUB_CLIENT_ERROR;
  }
  else if (!strcmp(optionName, OPTION_X509_CERT))
  {
    iotHubClientHandle->cert_path = az_span_create_from_str((char*)(uintptr_t)value);
    result = IOTHUB_CLIENT_OK;
  }
  else if (!strcmp(optionName, OPTION_X509_PRIVATE_KEY))
  {
    iotHubClientHandle->key_path = az_span_create_from_str((char*)(uintptr_t)value);
    result = IOTHUB_CLIENT_OK;
  }
  else
  {
    result = IOTHUB_CLIENT_ERROR;
  }

  return result;
}

IOTHUB_CLIENT_RESULT IoTHubDeviceClient_LL_SendEventAsync(
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle,
    IOTHUB_MESSAGE_HANDLE eventMessageHandle,
    IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback,
    void* userContextCallback)
{
  if (!iotHubClientHandle->connected)
  {
    az_hfsm_iot_x509_auth auth = (az_hfsm_iot_x509_auth){
      .cert = iotHubClientHandle->cert_path,
      .key = iotHubClientHandle->key_path,
    };

    // TODO:     az_hfsm_iot_hub_connect_data connect_data = (az_hfsm_iot_hub_connect_data){
    // TODO:       .auth = auth,
    // TODO:       .auth_type = AZ_HFSM_IOT_AUTH_X509,
    // TODO:       .client_id_buffer = AZ_SPAN_FROM_BUFFER(client_id_buffer),
    // TODO:       .username_buffer = AZ_SPAN_FROM_BUFFER(username_buffer),
    // TODO:       .password_buffer = AZ_SPAN_FROM_BUFFER(password_buffer),
    // TODO:     };
    // TODO:
    // TODO:     az_hfsm_send_event(
    // TODO:         (az_hfsm*)&hub_policy, (az_hfsm_event){ AZ_IOT_HUB_CONNECT_REQ, &connect_data
    // });
  }

  IOTHUB_MESSAGE_HANDLE_DATA* msg = eventMessageHandle;

  az_hfsm_iot_hub_telemetry_data telemetry_data = (az_hfsm_iot_hub_telemetry_data){
    .data = msg->payload,
    .out_packet_id = 0,
    .topic_buffer = msg->topic,
    .properties = NULL,
  };

  az_hfsm_send_event(
      (az_hfsm*)&iotHubClientHandle->hub_policy,
      (az_hfsm_event){ AZ_IOT_HUB_TELEMETRY_REQ, &telemetry_data });

  // telemetry_data.out_packet_id is set to correlate the PUBACK.
}

void IoTHubDeviceClient_LL_DoWork(IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle)
{
  // No-op: handled by the mosquitto-mqtt thread.
}

IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromString(const char* source)
{
  IOTHUB_MESSAGE_HANDLE_DATA* msg = malloc(sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
  char* topic_buffer = malloc(AZ_IOT_MAX_TOPIC_SIZE);
  char* properties_buffer = malloc(AZ_IOT_MAX_TOPIC_SIZE);

  if (msg != NULL && topic_buffer != NULL && properties_buffer != NULL
      && az_result_succeeded(az_iot_message_properties_init(
          &msg->properties, az_span_create(properties_buffer, AZ_IOT_MAX_TOPIC_SIZE), 0)))
  {
    msg->topic = az_span_create(topic_buffer, AZ_IOT_MAX_TOPIC_SIZE);
    msg->payload = az_span_create_from_str((char*)(uintptr_t)source);
  }
  else
  {
    IoTHubMessage_Destroy(msg);
    msg = NULL;
  }

  return msg;
}

void IoTHubMessage_Destroy(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
  IOTHUB_MESSAGE_HANDLE_DATA* msg = iotHubMessageHandle;
  if (msg != NULL)
  {
    if (az_span_ptr(msg->topic) != NULL)
    {
      free(az_span_ptr(msg->topic));
      msg->topic = AZ_SPAN_EMPTY;
    }

    if (az_span_ptr(msg->properties._internal.properties_buffer) != NULL)
    {
      free(az_span_ptr(msg->properties._internal.properties_buffer));
      msg->properties._internal.properties_buffer = AZ_SPAN_EMPTY;
    }

    free(msg);
    msg = NULL;
  }
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageId(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* messageId)
{
  return IoTHubMessage_SetProperty(iotHubMessageHandle, SYS_PROP_MESSAGE_ID, messageId);
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetCorrelationId(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* correlationId)
{
  return IoTHubMessage_SetProperty(iotHubMessageHandle, SYS_PROP_CORRELATION_ID, correlationId);
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentTypeSystemProperty(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* contentType)
{
  return IoTHubMessage_SetProperty(iotHubMessageHandle, SYS_PROP_CONTENT_TYPE, contentType);
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentEncodingSystemProperty(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* contentEncoding)
{
  return IoTHubMessage_SetProperty(iotHubMessageHandle, SYS_PROP_CONTENT_ENCODING, contentEncoding);
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetProperty(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* key,
    const char* value)
{
  IOTHUB_MESSAGE_HANDLE_DATA* msg = iotHubMessageHandle;

  if (az_result_failed(az_iot_message_properties_append(
          &msg->properties,
          az_span_create_from_str((char*)(uintptr_t)key),
          az_span_create_from_str((char*)(uintptr_t)value))))
  {
    return IOTHUB_MESSAGE_ERROR;
  }

  return IOTHUB_MESSAGE_OK;
}
