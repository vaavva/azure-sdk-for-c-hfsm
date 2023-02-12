/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_provisioning_hfsm.c
 * @brief HFSM for Azure IoT Retry
 *
 * @details Implements connectivity and fault handling for an IoT Client
 */

#include <stdint.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/iot/internal/az_iot_retry_hfsm.h>

#include <azure/core/_az_cfg.h>


AZ_NODISCARD az_hfsm_iot_retry_policy_options az_hfsm_iot_retry_policy_options_default()
{
  return (az_hfsm_iot_retry_policy_options) 
  {
    .secondary_credential = NULL
  };
}

AZ_NODISCARD az_result az_hfsm_iot_retry_policy_initialize(
    az_hfsm_iot_retry_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* outbound_policy,
    az_hfsm_iot_auth_type auth_type,
    az_hfsm_iot_auth* primary_credential,
    az_span username_buffer,
    az_span password_buffer,
    az_span client_id_buffer,
    az_hfsm_iot_retry_policy_options const* options)
{
  policy->_internal.options
    = options == NULL ? az_hfsm_iot_retry_policy_options_default() : *options;

  policy->_internal.policy.outbound_policy = outbound_policy;
  policy->_internal.policy.pipeline = pipeline;

  policy->_internal.auth_type = auth_type;
  policy->_internal.primary_credential = *primary_credential;
  policy->_internal.username_buffer = username_buffer;
  policy->_internal.password_buffer = password_buffer;
  policy->_internal.client_id_buffer = client_id_buffer;

  return AZ_OK;
}