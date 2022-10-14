// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license
// information.

/**
 * @brief Common C-SDK Compat API implementations.
 *
 */
#include <malloc.h>
#include <string.h>
#include <azure/az_iot.h>
#include <azure/core/az_mqtt.h>
#include <azure/iot/internal/az_iot_hub_hfsm.h>
#include <azure/iot/internal/az_iot_provisioning_hfsm.h>
#include <azure/iot/internal/az_iot_retry_hfsm.h>

#include <azure/iot/compat/internal/az_compat_csdk.h>

#define ENOMEM 12 /* Out of memory */
#define EINVAL 22 /* Invalid argument */

int mallocAndStrcpy_s(char** destination, const char* source)
{
  int result;
  int copied_result;
  /*Codes_SRS_CRT_ABSTRACTIONS_99_036: [destination parameter or source parameter is NULL, the error
   * code returned shall be EINVAL and destination shall not be modified.]*/
  if ((destination == NULL) || (source == NULL))
  {
    /*If strDestination or strSource is a NULL pointer[...]these functions return EINVAL */
    result = EINVAL;
  }
  else
  {
    size_t l = strlen(source);
    char* temp = (char*)malloc(l + 1);

    /*Codes_SRS_CRT_ABSTRACTIONS_99_037: [Upon failure to allocate memory for the destination, the
     * function will return ENOMEM.]*/
    if (temp == NULL)
    {
      result = ENOMEM;
    }
    else
    {
      *destination = temp;
      /*Codes_SRS_CRT_ABSTRACTIONS_99_039: [mallocAndstrcpy_s shall copy the contents in the address
       * source, including the terminating null character into location specified by the destination
       * pointer after the memory allocation.]*/
      strcpy(*destination, source);
      result = 0;
    }
  }
  return result;
}

const char* az_result_string(az_result result)
{
  const char* result_str;

  switch (result)
  {
    case AZ_ERROR_NOT_IMPLEMENTED:
      result_str = "AZ_ERROR_NOT_IMPLEMENTED";
      break;

    case AZ_ERROR_OUT_OF_MEMORY:
      result_str = "AZ_ERROR_OUT_OF_MEMORY";
      break;

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

void az_sdk_log_callback(az_log_classification classification, az_span message)
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
    case AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP:
      class_str = "AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP";
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

bool az_sdk_log_filter_callback(az_log_classification classification)
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
