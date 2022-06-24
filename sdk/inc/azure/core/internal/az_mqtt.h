// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief This header defines the types and functions your application uses to leverage MQTT pub/sub
 * functionality.
 * 
 * @details For more details on Azure IoT MQTT requirements please see 
 * https://docs.microsoft.com/azure/iot-hub/iot-hub-mqtt-support.
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
#include <azure/core/az_hfsm.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

// HFSM_TODO: we may want to add enums for the various MQTT status codes. These could be used to 
//            simplify logging.

typedef void* az_mqtt_impl;

#define AZ_MQTT_KEEPALIVE_SECONDS 240

typedef struct
{
  /**
   * The CA Trusted Roots span interpretable by the underlying MQTT implementation.
   */
  az_span certificate_authority_trusted_roots;
  az_span client_certificate;
  az_span client_private_key;
} az_mqtt_options;

/**
 * @brief Azure MQTT HFSM.
 * @details Derives from az_hfsm.
 * 
 */
typedef struct
{
  az_span host;
  int16_t port;
  az_span username;
  az_span password;
  az_span client_id;

  struct
  {
    az_hfsm hfsm;
    az_hfsm_dispatch iot_client;
    az_mqtt_impl mqtt;
    az_mqtt_options options;
  } _internal;
} az_mqtt_hfsm_type;

/**
 * @brief Azure MQTT HFSM event types.
 * 
 */
enum az_hfsm_event_type_mqtt
{
  /// MQTT Connect Request event.
  AZ_HFSM_MQTT_EVENT_CONNECT_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 1),

  /// MQTT Connect Response event.
  AZ_HFSM_MQTT_EVENT_CONNECT_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 2),

  AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 3),
  
  AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 4),

  AZ_HFSM_MQTT_EVENT_PUB_RECV_IND = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 5),

  AZ_HFSM_MQTT_EVENT_PUB_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 6),
  
  AZ_HFSM_MQTT_EVENT_PUBACK_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 7),

  AZ_HFSM_MQTT_EVENT_SUB_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 8),

  AZ_HFSM_MQTT_EVENT_SUBACK_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 9),
};

typedef struct {
  az_span topic;
  az_span payload;
  int8_t qos;
  int32_t* id;
} az_hfsm_mqtt_pub_data;

typedef struct {
  int32_t* id;
} az_hfsm_mqtt_puback_data;

typedef struct {
  int32_t connack_reason;
} az_hfsm_mqtt_connect_data;

AZ_NODISCARD az_mqtt_options az_mqtt_options_default();

/**
 * @brief 
 * 
 * @param mqtt_hfsm The #az_hfsm MQTT state machine instance.
 * @param iot_client The IoT client state machine dispatcher that will receive events from this 
 *                   machine.
 * @param host 
 * @param port 
 * @param username 
 * @param password 
 * @param client_id 
 * @param options 
 * @return AZ_NODISCARD 
 */
AZ_NODISCARD az_result az_mqtt_initialize(
  az_mqtt_hfsm_type* mqtt_hfsm,
  az_hfsm_dispatch* iot_client,
  az_span host,
  int16_t port,
  az_span username,
  az_span password,
  az_span client_id,
  az_mqtt_options const* options);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_H
