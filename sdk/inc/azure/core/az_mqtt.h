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
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT_H
#define _az_MQTT_H

#include <azure/core/az_config.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

// HFSM_TODO: we may want to add enums for the various MQTT status codes. These could be used to
//            simplify logging.

// MQTT library handle (type defined by implementation)
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

typedef struct
{
  // Derived from az_policy which is a kind of az_hfsm.
  struct
  {
    // HFSM_DESIGN: We could have different definitions for az_mqtt_impl to support additional
    //              memory reserved for the implementation:
    az_mqtt_impl mqtt;
    az_mqtt_options options;
  } _internal;
} az_mqtt;

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

AZ_NODISCARD az_result az_mqtt_outbound_pub(az_mqtt* mqtt, az_mqtt_pub_data pub_data);

typedef struct
{
  az_span topic;
  az_span payload;
  int8_t qos;
  int32_t id;
} az_mqtt_recv_data;

AZ_NODISCARD az_result az_mqtt_inbound_recv(az_mqtt* mqtt, az_mqtt_recv_data recv_data);

typedef struct
{
  int32_t id;
} az_mqtt_puback_data;

AZ_NODISCARD az_result az_mqtt_inbound_puback(az_mqtt* mqtt, az_mqtt_puback_data puback_data);

typedef struct
{
  az_span topic_filter;
  int8_t qos;
  int32_t out_id;
} az_mqtt_sub_data;

AZ_NODISCARD az_result az_mqtt_outbound_sub(az_mqtt* mqtt, az_mqtt_sub_data sub_data);

typedef struct
{
  int32_t id;
} az_mqtt_suback_data;

AZ_NODISCARD az_result az_mqtt_inbound_suback(az_mqtt* mqtt, az_mqtt_suback_data suback_data);

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

AZ_NODISCARD az_result az_mqtt_outbound_connect(az_mqtt* mqtt, az_mqtt_connect_data connect_data);

typedef struct
{
  int32_t connack_reason;
} az_mqtt_connack_data;

AZ_NODISCARD az_result az_mqtt_inbound_connack(az_mqtt* mqtt, az_mqtt_connack_data connack_data);

typedef struct
{
  int32_t disconnect_reason;
  bool disconnect_requested;
} az_mqtt_disconnect_data;

AZ_NODISCARD az_result
az_mqtt_inbound_disconnect(az_mqtt* mqtt, az_mqtt_disconnect_data disconnect_data);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_H
