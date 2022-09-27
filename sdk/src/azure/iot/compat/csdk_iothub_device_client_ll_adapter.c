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

static const size_t csdk_compat_max_topic_size = 128;

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

typedef struct IOTHUB_CLIENT_CORE_LL_HANDLE_DATA_TAG
{
  // Endpoint information
  az_span hub_endpoint;
  az_span device_id;
  az_span ca_path;
  az_span cert_path;
  az_span key_path;

  // HFSM Advanced Application
  az_hfsm_policy app_policy; // HFSM_TODO: rename to compat_client_policy
  az_platform_mutex disconnect_mutex;

  // Provisioning
  az_hfsm_pipeline prov_pipeline;
  az_hfsm_iot_provisioning_policy prov_policy;
  az_iot_provisioning_client prov_client;
  az_hfsm_mqtt_policy prov_mqtt_policy;

  // Hub
  az_hfsm_pipeline hub_pipeline;
  az_hfsm_iot_hub_policy hub_policy;
  az_iot_hub_client hub_client;
  az_hfsm_mqtt_policy hub_mqtt_policy;
} IOTHUB_CLIENT_CORE_LL_HANDLE_DATA;

typedef struct IOTHUB_MESSAGE_HANDLE_DATA_TAG
{
  az_span topic;
  az_span payload;
} IOTHUB_MESSAGE_HANDLE_DATA;

const TRANSPORT_PROVIDER* MQTT_Protocol(void) { return (TRANSPORT_PROVIDER*)0x4D515454; }

static void az_sdk_log_callback(az_log_classification classification, az_span message);
static bool az_sdk_log_filter_callback(az_log_classification classification);

#define LOG_APP "\x1B[34mAPP: \x1B[0m"
#define LOG_SDK "\x1B[33mSDK: \x1B[0m"

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
  printf(LOG_APP "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

static az_result hub_initialize(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client)
{
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &client->hub_pipeline,
      (az_hfsm_policy*)&client->app_policy,
      (az_hfsm_policy*)&client->hub_mqtt_policy));

  _az_RETURN_IF_FAILED(
      az_iot_hub_client_init(&client->hub_client, client->hub_endpoint, client->device_id, NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_hub_policy_initialize(
      &client->hub_policy,
      &client->hub_pipeline,
      &client->app_policy,
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

// C-SDK Compat layer

int IoTHub_Init() { _az_RETURN_IF_FAILED(az_mqtt_init()); }

void IoTHub_Deinit()
{
  if (az_result_failed(az_mqtt_deinit()))
  {
    az_platform_critical_error();
  }
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE IoTHubDeviceClient_LL_Create(const IOTHUB_CLIENT_CONFIG* config)
{
  _az_PRECONDITION(config->protocol == (IOTHUB_CLIENT_TRANSPORT_PROVIDER)MQTT_Protocol);

  IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client = malloc(sizeof(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA));
  return client;
}

void IoTHubDeviceClient_LL_Destroy(IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle)
{
  free(iotHubClientHandle);
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
    az_log_set_message_callback(az_sdk_log_callback);
    az_log_set_classification_filter_callback(az_sdk_log_filter_callback);
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
    // HFSM_TODO: not implemented.
    result = IOTHUB_CLIENT_OK;
  }
  else if (!strcmp(optionName, OPTION_X509_PRIVATE_KEY))
  {
    // HFSM_TODO: not implemented.
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

void IoTHubDeviceClient_LL_DoWork(IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle) {}

IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromString(const char* source) 
{
    IOTHUB_MESSAGE_HANDLE_DATA* msg = malloc(sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
    char* topic_buffer = malloc(csdk_compat_max_topic_size);
    char* payload_buffer = malloc(sizeof(source) + 1);

    if (msg == NULL || topic_buffer == NULL || payload_buffer == NULL)
    {
        IoTHubMessage_Destroy(msg);
        msg = NULL;
    }
    else
    {
        strcpy(payload_buffer, source);
        msg->topic = az_span_create(topic_buffer, csdk_compat_max_topic_size);
        msg->payload = az_span_create(payload_buffer, sizeof(source) + 1);
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

        if (az_span_ptr(msg->payload) != NULL)
        {
            free(az_span_ptr(msg->payload));
            msg->payload = AZ_SPAN_EMPTY;
        }

        free(msg);
        msg = NULL;
    }
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageId(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* messageId)
{
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetCorrelationId(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* correlationId)
{
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentTypeSystemProperty(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* contentType)
{
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentEncodingSystemProperty(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* contentEncoding)
{
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetProperty(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* key,
    const char* value)
{

}

