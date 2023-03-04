// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_mqtt.h
 *
 * @brief This header defines the types and functions your application uses to leverage MQTT pub/sub
 * functionality.
 *
 * @details For more details on Azure IoT MQTT requirements please see
 * https://docs.microsoft.com/azure/iot-hub/iot-hub-mqtt-support.
 * - Outbound APIs will be called by the SDK to send data over the network.
 * - Inbound API calls must be called by the MQTT implementation to send data towards the SDK.
 *
 * @note Object lifetime: all APIs have run-to-completion semantics. Data passed into the APIs
 *       is owned by the API for the duration of the call.
 *
 * @note API I/O model: It is generally expected that all operations are asynchronous. An outbound
 *       call should not block, and may occur on the same call-stack, as a result of an inbound
 *       call.
 *
 * @note The SDK expects that the network stack is stalled for the duration of the API calls.
 *
 * @note All API implementations must return az_result.
 * We recommend using `_az_RESULT_MAKE_ERROR(_az_FACILITY_IOT_MQTT, mqtt_stack_ret);` if compatible.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT_H
#define _az_MQTT_H

#include <azure/core/az_config.h>
#include <azure/core/az_context.h>
#include <azure/core/az_credentials_x509.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_event.h>
#include <stdbool.h>
#include <stdint.h>

#if defined(TRANSPORT_PAHO)
    #include <azure/platform/az_mqtt_paho.h>
#elif defined(TRANSPORT_MOSQUITTO)
    #include <azure/platform/az_mqtt_mosquitto.h>
#else
    #include <azure/platform/az_mqtt_notransport.h>
#endif

#include <azure/core/_az_cfg_prefix.h>

// HFSM_TODO: we may want to add enums for the various MQTT status codes. These could be used to
//            simplify logging.


typedef struct
{
  az_span topic;
  az_span payload;
  int8_t qos;

  // The MQTT stack should set this ID upon returning.
  int32_t out_id;
} az_mqtt_pub_data;

typedef struct
{
  az_span topic;
  az_span payload;
  int8_t qos;
  int32_t id;
} az_mqtt_recv_data;

typedef struct
{
  int32_t id;
} az_mqtt_puback_data;

typedef struct
{
  az_span topic_filter;
  int8_t qos;
  int32_t out_id;
} az_mqtt_sub_data;

typedef struct
{
  int32_t id;
} az_mqtt_suback_data;

typedef struct
{
  az_span host;
  int16_t port;
  az_span username;
  az_span password;
  az_span client_id;

  az_credential_x509 certificate;
} az_mqtt_connect_data;

typedef struct
{
  int32_t connack_reason;
  bool tls_authentication_error;
} az_mqtt_connack_data;

typedef struct
{
  bool tls_authentication_error;
  bool disconnect_requested;
} az_mqtt_disconnect_data;

/**
 * @brief Azure MQTT HFSM event types.
 *
 */
// HFSM_TODO: az_log_classification_iot uses _az_FACILITY_IOT_MQTT up to ID 2.
enum az_event_type_mqtt
{
  /// MQTT Connect Request event.
  AZ_MQTT_EVENT_CONNECT_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 10),

  /// MQTT Connect Response event.
  AZ_MQTT_EVENT_CONNECT_RSP = _az_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 11),

  AZ_MQTT_EVENT_DISCONNECT_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 12),

  AZ_MQTT_EVENT_DISCONNECT_RSP = _az_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 13),

  AZ_MQTT_EVENT_PUB_RECV_IND = _az_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 14),

  AZ_MQTT_EVENT_PUB_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 15),

  AZ_MQTT_EVENT_PUBACK_RSP = _az_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 16),

  AZ_MQTT_EVENT_SUB_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 17),

  AZ_MQTT_EVENT_SUBACK_RSP = _az_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 18),
};

// Porting 1. The following functions must be called by the implementation when data is received:

AZ_NODISCARD AZ_INLINE az_result az_mqtt_inbound_recv(az_mqtt* mqtt, az_mqtt_recv_data* recv_data)
{
  if (!mqtt->_internal.platform_mqtt._inbound_handler)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  mqtt->_internal.platform_mqtt._inbound_handler(
      mqtt, (az_event){ .type = AZ_MQTT_EVENT_PUB_RECV_IND, .data = recv_data });
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result
az_mqtt_inbound_connack(az_mqtt* mqtt, az_mqtt_connack_data* connack_data)
{
  if (!mqtt->_internal.platform_mqtt._inbound_handler)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  mqtt->_internal.platform_mqtt._inbound_handler(
      mqtt, (az_event){ .type = AZ_MQTT_EVENT_CONNECT_RSP, .data = connack_data });
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result
az_mqtt_inbound_suback(az_mqtt* mqtt, az_mqtt_suback_data* suback_data)
{
  if (!mqtt->_internal.platform_mqtt._inbound_handler)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  mqtt->_internal.platform_mqtt._inbound_handler(
      mqtt, (az_event){ .type = AZ_MQTT_EVENT_SUBACK_RSP, .data = suback_data });
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result
az_mqtt_inbound_puback(az_mqtt* mqtt, az_mqtt_puback_data* puback_data)
{
  if (!mqtt->_internal.platform_mqtt._inbound_handler)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  mqtt->_internal.platform_mqtt._inbound_handler(
      mqtt, (az_event){ .type = AZ_MQTT_EVENT_PUBACK_RSP, .data = puback_data });
  return AZ_OK;
}

AZ_NODISCARD AZ_INLINE az_result
az_mqtt_inbound_disconnect(az_mqtt* mqtt, az_mqtt_disconnect_data* disconnect_data)
{
  if (!mqtt->_internal.platform_mqtt._inbound_handler)
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }

  mqtt->_internal.platform_mqtt._inbound_handler(
      mqtt, (az_event){ .type = AZ_MQTT_EVENT_DISCONNECT_RSP, .data = disconnect_data });
  return AZ_OK;
}

// Porting 2. The following functions must be implemented and will be called by the SDK to
//            send data:

AZ_NODISCARD az_mqtt_options az_mqtt_options_default();
AZ_NODISCARD az_result az_mqtt_init(az_mqtt* mqtt, az_mqtt_options const* options);
AZ_NODISCARD az_result
az_mqtt_outbound_connect(az_mqtt* mqtt, az_context* context, az_mqtt_connect_data* connect_data);
AZ_NODISCARD az_result
az_mqtt_outbound_sub(az_mqtt* mqtt, az_context* context, az_mqtt_sub_data* sub_data);
AZ_NODISCARD az_result
az_mqtt_outbound_pub(az_mqtt* mqtt, az_context* context, az_mqtt_pub_data* pub_data);
AZ_NODISCARD az_result az_mqtt_outbound_disconnect(az_mqtt* mqtt, az_context* context);
AZ_NODISCARD az_result az_mqtt_wait_for_event(az_mqtt* mqtt, int32_t timeout);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_H
