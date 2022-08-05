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

#include <azure/iot/internal/az_iot_hub_hfsm.h>

#include <azure/core/_az_cfg.h>


AZ_NODISCARD az_hfsm_iot_hub_policy_options az_hfsm_iot_hub_policy_options_default()
{
  return (az_hfsm_iot_hub_policy_options){ ._reserved = 0 };
}

AZ_NODISCARD az_result az_hfsm_iot_hub_policy_initialize(
    az_hfsm_iot_hub_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_hfsm_policy* outbound_policy,
    az_iot_hub_client* hub_client,
    az_hfsm_iot_hub_policy_options const* options)
{
  policy->_internal.options
      = options == NULL ? az_hfsm_iot_hub_policy_options_default() : *options;

  policy->_internal.policy.outbound = outbound_policy;
  policy->_internal.policy.inbound = inbound_policy;

  policy->_internal.pipeline = pipeline;

  policy->_internal.hub_client = hub_client;

  return AZ_OK;  
}
