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
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT_H
#define _az_MQTT_H

#include <azure/core/az_config.h>
#include <azure/core/az_hfsm.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

// HFSM_TODO: we may want to add enums for the various MQTT status codes. These could be used to
//            simplify logging.

// MQTT library handle (type defined by implementation)
typedef void* az_mqtt_impl;

typedef struct
{
  az_span topic;
  az_span payload;
  int8_t qos;
  int32_t out_id;
} az_hfsm_mqtt_pub_data;

typedef struct
{
  az_span topic;
  az_span payload;
  int8_t qos;
  int32_t id;
} az_hfsm_mqtt_recv_data;

typedef struct
{
  int32_t id;
} az_hfsm_mqtt_puback_data;

typedef struct
{
  az_span topic_filter;
  int8_t qos;
  int32_t out_id;
} az_hfsm_mqtt_sub_data;

typedef struct
{
  int32_t id;
} az_hfsm_mqtt_suback_data;

typedef struct
{
  az_span host;
  int16_t port;
  az_span username;
  az_span password;
  az_span client_id;

  az_span client_certificate;
  az_span client_private_key;

} az_hfsm_mqtt_connect_data;

typedef struct
{
  int32_t connack_reason;
} az_hfsm_mqtt_connack_data;

typedef struct
{
  int32_t disconnect_reason;
  bool disconnect_requested;
} az_hfsm_mqtt_disconnect_data;

typedef struct
{
  /**
   * The CA Trusted Roots span interpretable by the underlying MQTT implementation.
   */
  az_span certificate_authority_trusted_roots;
} az_hfsm_mqtt_policy_options;

/**
 * @brief Azure MQTT HFSM.
 * @details Derives from az_hfsm.
 *
 */
typedef struct
{
  // Derived from az_policy which is a kind of az_hfsm.
  struct
  {
    az_hfsm_policy policy;

    // Extension point for the implementation.
    // HFSM_DESIGN: We could have different definitions for az_mqtt_impl to support additional
    //              memory reserved for the implementation:
    az_mqtt_impl mqtt;
    az_hfsm_mqtt_policy_options options;
  } _internal;
} az_hfsm_mqtt_policy;

/**
 * @brief Azure MQTT HFSM event types.
 *
 */
// HFSM_TODO: az_log_classification_iot uses _az_FACILITY_IOT_MQTT up to ID 2.
enum az_hfsm_event_type_mqtt
{
  /// MQTT Connect Request event.
  AZ_HFSM_MQTT_EVENT_CONNECT_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 10),

  /// MQTT Connect Response event.
  AZ_HFSM_MQTT_EVENT_CONNECT_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 11),

  AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 12),

  AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 13),

  AZ_HFSM_MQTT_EVENT_PUB_RECV_IND = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 14),

  AZ_HFSM_MQTT_EVENT_PUB_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 15),

  AZ_HFSM_MQTT_EVENT_PUBACK_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 16),

  AZ_HFSM_MQTT_EVENT_SUB_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 17),

  AZ_HFSM_MQTT_EVENT_SUBACK_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 18),
};

AZ_NODISCARD az_hfsm_mqtt_policy_options az_hfsm_mqtt_policy_options_default();

/**
 * @brief Initializes the MQTT Platform Layer.
 *
 * @param mqtt_hfsm The #az_hfsm MQTT state machine instance.
 * @param parent The IoT client state machine dispatcher that will receive events from this
 *                   machine.
 * @param client_id
 * @param options
 * @return #az_result
 */
AZ_NODISCARD az_result az_mqtt_initialize(
    az_hfsm_mqtt_policy* mqtt_hfsm,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_hfsm_mqtt_policy_options const* options);

/**
 * @brief Initialization of the MQTT Stack.
 * @details Must be called at most once by the application before the MQTT stack is used.
 *
 * @return #az_result
 */
AZ_NODISCARD az_result az_mqtt_init();

/**
 * @brief Deinitialization of the MQTT Stack.
 * @details Must be called at most once by the application to release MQTT stack resources.
 *
 * @return #az_result
 */
AZ_NODISCARD az_result az_mqtt_deinit();

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_H
