// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief This header defines the types and functions your application uses to leverage MQTT pub/sub
 * functionality.
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

// CPOP_TODO: we may want to add enums for the various MQTT status codes. These could be used to 
//            simplify logging.

/**
 * @brief Azure MQTT HFSM.
 * @details Derives from az_hfsm.
 * 
 */
typedef struct
{
  struct
  {
    az_hfsm hfsm;
    az_span host;
    int16_t port;
    void* tls_settings;
    az_span username;
    az_span password;
    az_span client_id;
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

//CPOP_TODO: add standard Options support. tls_settings should be in options.

AZ_NODISCARD az_result az_mqtt_initialize(
  az_mqtt_hfsm_type* mqtt_hfsm,
  az_span host,
  int16_t port,
  void* tls_settings,
  az_span username,
  az_span password,
  az_span client_id);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_H
