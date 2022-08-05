/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief Hierarchical Finite State Machine (HFSM) for Azure IoT Operations.
 *
 * @details Implements fault handling for Azure Device Provisioning + IoT Hub operations.
 */
#ifndef _az_IOT_PROVISIONING_HFSM_H
#define _az_IOT_PROVISIONING_HFSM_H

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/az_config.h>
#include <azure/core/az_hfsm.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <azure/az_iot.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  int32_t _reserved;
} az_hfsm_iot_provisioning_policy_options;

typedef struct
{
  struct
  {
    az_hfsm_policy policy;
    az_hfsm_pipeline* pipeline;
    az_iot_provisioning_client* provisioning_client;
    az_hfsm_iot_provisioning_policy_options options;
  } _internal;
} az_hfsm_iot_provisioning_policy;

enum az_hfsm_event_type_mqtt
{
  // HFSM_DESIGN: Start / Stop are not very useful for DPS given that the service offers a single 
  //              API today: register_device.

  /// Connects the Provisioning HFSM.
  AZ_IOT_PROVISIONING_START = _az_HFSM_MAKE_EVENT(_az_FACILITY_PROVISIONING_HFSM, 0),

  /// Disconnects the Provisioning HFSM.
  AZ_IOT_PROVISIONING_STOP = _az_HFSM_MAKE_EVENT(_az_FACILITY_PROVISIONING_HFSM, 1),

  /// Device Registration Request 
  AZ_IOT_PROVISIONING_REGISTER_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_PROVISIONING_HFSM, 2),

  /// Device Registration Response
  AZ_IOT_PROVISIONING_REGISTER_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_PROVISIONING_HFSM, 3),
};

AZ_NODISCARD az_hfsm_iot_provisioning_policy_options
az_hfsm_iot_provisioning_policy_options_default();

AZ_NODISCARD az_result az_hfsm_iot_provisioning_policy_initialize(
    az_hfsm_iot_provisioning_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_hfsm_policy* outbound_policy,
    az_iot_provisioning_client* provisioning_client,
    az_hfsm_iot_provisioning_policy_options const* options);

#endif //_az_IOT_PROVISIONING_HFSM_H
