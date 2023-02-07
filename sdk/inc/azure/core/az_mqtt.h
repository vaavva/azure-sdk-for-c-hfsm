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
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/core/az_hfsm_pipeline.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

// HFSM_TODO: we may want to add enums for the various MQTT status codes. These could be used to
//            simplify logging.

// MQTT library handle (type defined by implementation)
typedef struct az_mqtt az_mqtt;
typedef void* az_mqtt_impl;
typedef void* az_mqtt_impl_options;

typedef struct
{
  /**
   * The CA Trusted Roots span interpretable by the underlying MQTT implementation.
   */
  az_span certificate_authority_trusted_roots;
  // HFSM_DESIGN: Extension point to add MQTT stack specific options here.
  az_mqtt_impl_options implementation_specific_options;
} az_mqtt_options;

AZ_NODISCARD az_mqtt_options az_mqtt_options_default();

AZ_NODISCARD az_result az_mqtt_init(az_mqtt* out_mqtt, az_mqtt_options const* options);

typedef struct
{
  az_span topic;
  az_span payload;
  int8_t qos;

  // The MQTT stack should set this ID upon returning.
  int32_t out_id;
} az_mqtt_pub_data;

AZ_NODISCARD az_result
az_mqtt_outbound_pub(az_mqtt* mqtt, az_mqtt_pub_data pub_data, az_context context);

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

AZ_NODISCARD az_result
az_mqtt_outbound_sub(az_mqtt* mqtt, az_mqtt_sub_data sub_data, az_context context);

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

  az_span client_certificate;
  az_span client_private_key;
} az_mqtt_connect_data;

AZ_NODISCARD az_result
az_mqtt_outbound_connect(az_mqtt* mqtt, az_mqtt_connect_data connect_data, az_context context);

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

typedef void (*az_mqtt_connack_handler)(az_mqtt* mqtt, az_mqtt_connack_data data);
typedef void (*az_mqtt_recv_handler)(az_mqtt* mqtt, az_mqtt_recv_data recv_data);
typedef void (*az_mqtt_puback_handler)(az_mqtt* mqtt, az_mqtt_puback_data puback_data);
typedef void (*az_mqtt_suback_handler)(az_mqtt* mqtt, az_mqtt_suback_data puback_data);
typedef void (*az_mqtt_disconnect_handler)(az_mqtt* mqtt, az_mqtt_disconnect_data data);

struct az_mqtt
{
  struct
  {
    az_mqtt_connack_handler* _connack_handler;
    az_mqtt_recv_handler* _recv_handler;
    az_mqtt_puback_handler* _puback_handler;
    az_mqtt_suback_handler* _suback_handler;
    az_mqtt_disconnect_handler* _disconnect_handler;
  } _internal;

  az_mqtt_impl mqtt;
  az_mqtt_options options;
};

AZ_NODISCARD AZ_INLINE az_result az_mqtt_inbound_recv(az_mqtt* mqtt, az_mqtt_recv_data recv_data)
{
  if (mqtt->_internal._recv_handler)
  {
    return mqtt->_internal._recv_handler(mqtt, recv_data);
  }
  else
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }
}

AZ_NODISCARD AZ_INLINE az_result
az_mqtt_inbound_connack(az_mqtt* mqtt, az_mqtt_connack_data connack_data)
{
  if (mqtt->_internal._connack_handler)
  {
    return mqtt->_internal._connack_handler(mqtt, connack_data);
  }
  else
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }
}

AZ_NODISCARD AZ_INLINE az_result
az_mqtt_inbound_suback(az_mqtt* mqtt, az_mqtt_suback_data suback_data)
{
  if (mqtt->_internal._suback_handler)
  {
    return mqtt->_internal._suback_handler(mqtt, suback_data);
  }
  else
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }
}

AZ_NODISCARD AZ_INLINE az_result
az_mqtt_inbound_puback(az_mqtt* mqtt, az_mqtt_puback_data puback_data)
{
  if (mqtt->_internal._puback_handler)
  {
    return mqtt->_internal._puback_handler(mqtt, puback_data);
  }
  else
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }
}

AZ_NODISCARD AZ_INLINE az_result
az_mqtt_inbound_disconnect(az_mqtt* mqtt, az_mqtt_disconnect_data disconnect_data)
{
  if (mqtt->_internal._disconnect_handler)
  {
    return mqtt->_internal._disconnect_handler(mqtt, disconnect_data);
  }
  else
  {
    return AZ_ERROR_NOT_IMPLEMENTED;
  }
}

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_H
