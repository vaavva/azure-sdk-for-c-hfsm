/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief Hierarchical Finite State Machine (HFSM) for Azure IoT Operations.
 * 
 * @details Implements fault handling for Azure Device Provisioning + IoT Hub operations.
 */
#ifndef _az_IOT_HFSM_H
#define _az_IOT_HFSM_H

#include <stdint.h>
#include <stdbool.h>

#include <azure/az_core.h>
#include <azure/core/az_hfsm.h>
#include <azure/iot/az_iot_common.h>

typedef struct
{
  struct
  {
    az_hfsm hfsm;
    az_hfsm_dispatch* iot_hfsm;
  } _internal;
} az_iot_provisioning_hfsm_type;

/**
 * @brief Initializes an IoT HFSM object.
 * 
 * @param[out] iot_hfsm The #az_hfsm to use for this call.
 * @param[in] provisioning_hfsm The #az_hfsm implementing #az_iot_provisioning_client operations.
 * @param[in] hub_hfsm The #az_hfsm implementing #az_iot_hub_client operations.
 * @return An #az_result value indicating the result of the operation.
 */

/**
 * @brief Initializes an IoT Provisioning HFSM object.
 * 
 * @param[out] provisioning_hfsm The #az_hfsm to use for this call.
 * @param[in] iot_hfsm The parent, IoT orchestration #az_hfsm.
 */
AZ_NODISCARD az_result az_iot_provisioning_hfsm_initialize(
  az_iot_provisioning_hfsm_type* provisioning_hfsm, 
  az_hfsm_dispatch* iot_hfsm);

#endif //_az_IOT_HFSM_H
