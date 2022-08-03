/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief Hierarchical Finite State Machine (HFSM) for Azure IoT Operations.
 *
 * @details Implements fault handling for Azure Device Provisioning + IoT Hub operations.
 */
#ifndef _az_IOT_RETRY_HFSM_H
#define _az_IOT_RETRY_HFSM_H

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
  int _reserved;
} az_hfsm_iot_retry_policy_options;

typedef struct
{
  struct
  {
    az_hfsm_policy policy;
    az_hfsm_pipeline* provisioning_pipeline;
    az_hfsm_pipeline* hub_pipeline;
    az_hfsm_iot_retry_policy_options options;
  } _internal;
} az_hfsm_iot_retry_policy;

AZ_NODISCARD az_hfsm_iot_retry_policy_options az_hfsm_iot_retry_policy_options_default();

AZ_NODISCARD az_result az_hfsm_iot_retry_policy_initialize(
    az_hfsm_iot_retry_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_hfsm_iot_retry_policy_options const* options);

#endif //_az_IOT_RETRY_HFSM_H
