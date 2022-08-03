/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hub_hfsm.c
 * @brief HFSM for Azure IoT Hub Operations.
 *
 * @details Implements connectivity and fault handling for an IoT Hub Client
 */

#include <stdint.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/internal/

#include <azure/core/_az_cfg.h>

static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event);

// Hardcoded Azure IoT hierarchy structure
static az_hfsm_state_handler azure_iot_hfsm_get_parent(az_hfsm_state_handler child_state)
{

  return parent_state;
}


AZ_NODISCARD az_hfsm_iot_hub_policy_options az_hfsm_iot_hub_policy_options_default()
{
  return (az_hfsm_iot_hub_policy_options){ .reserved = 0 };
}

AZ_NODISCARD az_result az_hfsm_iot_hub_policy_initialize(
    az_hfsm_iot_hub_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_hfsm_policy* outbound_policy,
    az_iot_hub_client* hub_client,
    az_hfsm_iot_hub_policy_options const* options)
{
  
}
