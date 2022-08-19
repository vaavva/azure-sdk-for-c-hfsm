/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief Hierarchical Finite State Machine (HFSM) for Azure IoT Hub.
 *
 * @details Implements the client for Azure IoT Hub.
 */
#ifndef _az_IOT_HUB_HFSM_H
#define _az_IOT_HUB_HFSM_H

#include <azure/core/az_config.h>
#include <azure/core/az_hfsm.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/az_iot.h>
#include <azure/iot/internal/az_iot_retry_hfsm.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  az_hfsm_iot_auth_type auth_type;
  az_hfsm_iot_auth auth;

  az_span username_buffer;
  az_span password_buffer;
  az_span client_id_buffer;
} az_hfsm_iot_hub_connect_data;

typedef struct
{
  az_span data;
  az_iot_message_properties* properties;
  int32_t out_packet_id;

  az_span topic_buffer;
} az_hfsm_iot_hub_telemetry_data;

typedef az_iot_hub_client_method_request az_hfsm_iot_hub_method_request_data;

typedef struct
{
  az_span request_id;
  uint16_t status; 
  az_span payload;

  az_span topic_buffer;
} az_hfsm_iot_hub_method_response_data;

typedef struct
{
  int16_t port;
} az_hfsm_iot_hub_policy_options;

typedef struct
{
  struct
  {
    az_hfsm_policy policy;
    az_iot_hub_client* hub_client;
    az_hfsm_iot_hub_policy_options options;
  } _internal;
} az_hfsm_iot_hub_policy;

enum az_hfsm_event_type_hub_hfsm
{
  /// Connection control
  AZ_IOT_HUB_CONNECT_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_HUB_HFSM, 0),
  AZ_IOT_HUB_CONNECT_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_HUB_HFSM, 1),
  AZ_IOT_HUB_DISCONNECT_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_HUB_HFSM, 2),
  AZ_IOT_HUB_DISCONNECT_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_HUB_HFSM, 3),

  /// Telemetry
  AZ_IOT_HUB_TELEMETRY_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_HUB_HFSM, 4),
  
  // TODO: Remove - very hard to implement matching.
  AZ_IOT_HUB_TELEMETRY_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_HUB_HFSM, 5),

  /// Methods
  AZ_IOT_HUB_METHODS_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_HUB_HFSM, 6),
  AZ_IOT_HUB_METHODS_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_HUB_HFSM, 7),
};

AZ_NODISCARD az_hfsm_iot_hub_policy_options az_hfsm_iot_hub_policy_options_default();

AZ_NODISCARD az_result az_hfsm_iot_hub_policy_initialize(
    az_hfsm_iot_hub_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_hfsm_policy* outbound_policy,
    az_iot_hub_client* hub_client,
    az_hfsm_iot_hub_policy_options const* options);

#endif //_az_IOT_HUB_HFSM_H
