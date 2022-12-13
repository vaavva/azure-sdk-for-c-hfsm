// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_sm_device_orchestration.h
 *
 * @brief Definition for the Azure Device Orchestration state machine.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_SM_MQTT_WRAPPER_H
#define _az_IOT_SM_MQTT_WRAPPER_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_sm_provisioning_client.h>
#include <azure/core/az_mqtt.h>

#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief MQTT Client abstraction
 * 
 */
typedef struct
{
  struct
  {
    az_hfsm_mqtt_policy mqtt_policy;
  } _internal;
} az_iot_sm_mqtt_client;



#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_SM_MQTT_WRAPPER_H
