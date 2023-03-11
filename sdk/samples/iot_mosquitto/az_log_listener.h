// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_LOG_LISTENER_H
#define _az_LOG_LISTENER_H

#include <azure/az_core.h>
#include <azure/az_iot.h>
#include <azure/core/az_log.h>
// For HFSM ENTER/EXIT events.
#include <azure/core/internal/az_hfsm.h>

#include <stdio.h>

/*
Black:   \x1B[30m
Red:     \x1B[31m
Green:   \x1B[32m
Yellow:  \x1B[33m
Blue:    \x1B[34m
Magenta: \x1B[35m
Cyan:    \x1B[36m
White:   \x1B[37m
Reset:   \x1B[0m
*/

#define LOG_APP "\x1B[34mAPP: \x1B[0m"
#define LOG_APP_ERROR "\x1B[31mAPP: \x1B[0m"

#define LOG_SDK "\x1B[33mSDK: \x1B[0m"

AZ_INLINE char* az_result_to_string(az_result result)
{
  switch (result)
  {
    case AZ_OK:
      return "AZ_OK";
    case AZ_ERROR_CANCELED:
      return "AZ_ERROR_CANCELED";
    case AZ_ERROR_ARG:
      return "AZ_ERROR_ARG";
    case AZ_ERROR_NOT_ENOUGH_SPACE:
      return "AZ_ERROR_NOT_ENOUGH_SPACE";
    case AZ_ERROR_NOT_IMPLEMENTED:
      return "AZ_ERROR_NOT_IMPLEMENTED";
    case AZ_ERROR_ITEM_NOT_FOUND:
      return "AZ_ERROR_ITEM_NOT_FOUND";
    case AZ_ERROR_UNEXPECTED_CHAR:
      return "AZ_ERROR_UNEXPECTED_CHAR";
    case AZ_ERROR_UNEXPECTED_END:
      return "AZ_ERROR_UNEXPECTED_END";
    case AZ_ERROR_NOT_SUPPORTED:
      return "AZ_ERROR_NOT_SUPPORTED";
    case AZ_ERROR_DEPENDENCY_NOT_PROVIDED:
      return "AZ_ERROR_DEPENDENCY_NOT_PROVIDED";
    case AZ_ERROR_OUT_OF_MEMORY:
      return "AZ_ERROR_OUT_OF_MEMORY";
    case AZ_TIMEOUT:
      return "AZ_TIMEOUT";
    case AZ_ERROR_JSON_INVALID_STATE:
      return "AZ_ERROR_JSON_INVALID_STATE";
    case AZ_ERROR_JSON_NESTING_OVERFLOW:
      return "AZ_ERROR_JSON_NESTING_OVERFLOW";
    case AZ_ERROR_JSON_READER_DONE:
      return "AZ_ERROR_JSON_READER_DONE";
    default:
      return "UNKNOWN";
  }
}

#define LOG_AND_EXIT_IF_FAILED(exp)                                        \
  do                                                                       \
  {                                                                        \
    az_result const _az_result = (exp);                                    \
    if (az_result_failed(_az_result))                                      \
    {                                                                      \
      printf(                                                              \
          LOG_APP_ERROR "%s failed with error \x1B[31m0x%x (%s)\x1B[0m\n", \
          #exp,                                                            \
          _az_result,                                                      \
          az_result_to_string(_az_result));                                \
      return _az_result;                                                   \
    }                                                                      \
  } while (0)

AZ_INLINE void az_sdk_log_callback(az_log_classification classification, az_span message)
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
    case AZ_MQTT_EVENT_CONNECT_REQ:
      class_str = "AZ_MQTT_EVENT_CONNECT_REQ";
      break;
    case AZ_MQTT_EVENT_CONNECT_RSP:
      class_str = "AZ_MQTT_EVENT_CONNECT_RSP";
      break;
    case AZ_MQTT_EVENT_DISCONNECT_REQ:
      class_str = "AZ_MQTT_EVENT_DISCONNECT_REQ";
      break;
    case AZ_MQTT_EVENT_DISCONNECT_RSP:
      class_str = "AZ_MQTT_EVENT_DISCONNECT_RSP";
      break;
    case AZ_MQTT_EVENT_PUB_RECV_IND:
      class_str = "AZ_MQTT_EVENT_PUB_RECV_IND";
      break;
    case AZ_MQTT_EVENT_PUB_REQ:
      class_str = "AZ_MQTT_EVENT_PUB_REQ";
      break;
    case AZ_MQTT_EVENT_PUBACK_RSP:
      class_str = "AZ_MQTT_EVENT_PUBACK_RSP";
      break;
    case AZ_MQTT_EVENT_SUB_REQ:
      class_str = "AZ_MQTT_EVENT_SUB_REQ";
      break;
    case AZ_MQTT_EVENT_SUBACK_RSP:
      class_str = "AZ_MQTT_EVENT_SUBACK_RSP";
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
    // TODO: case AZ_IOT_PROVISIONING_REGISTER_REQ:
    //   class_str = "AZ_IOT_PROVISIONING_REGISTER_REQ";
    //   break;
    default:
      class_str = NULL;
  }

  // TODO: add thread ID.

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

#endif //_az_LOG_LISTENER_H
