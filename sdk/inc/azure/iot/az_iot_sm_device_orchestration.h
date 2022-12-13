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

#ifndef _az_IOT_SM_DEVICE_ORCHESTRATION_H
#define _az_IOT_SM_DEVICE_ORCHESTRATION_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_sm_provisioning_client.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  bool _unused;
} az_iot_sm_device_orchestration_options;

typedef struct
{
  struct
  {
    az_iot_sm_provisioning_client provisioning_sm;
  } _internal;
} az_iot_sm_device_orchestration;

AZ_NODISCARD az_iot_sm_device_orchestration_options
az_iot_sm_device_orchestration_options_default();

AZ_NODISCARD az_result
az_iot_sm_device_orchestration_initialize(az_iot_sm_device_orchestration* client);

typedef az_result (*az_iot_sm_device_orchestration_register_status_callback)(
    az_iot_sm_device_orchestration* client);

typedef az_result (*az_iot_sm_device_orchestration_register_result_callback)(
    az_iot_sm_device_orchestration* client);

AZ_NODISCARD az_result az_iot_sm_device_orchestration_register(
    az_iot_sm_device_orchestration* client,
    az_iot_sm_device_orchestration_register_status_callback* status_callback,
    az_iot_sm_device_orchestration_register_result_callback* result_callback);

AZ_NODISCARD az_result
az_iot_sm_device_orchestration_register_cancel(az_iot_sm_device_orchestration* client);

AZ_NODISCARD az_result
az_iot_sm_device_orchestration_register_get_status(az_iot_sm_device_orchestration* client);

AZ_NODISCARD az_result
az_iot_sm_device_orchestration_register_get_result(az_iot_sm_device_orchestration* client);

#ifdef TRANSPORT_MQTT_SYNC
AZ_NODISCARD az_result
az_iot_sm_device_orchestration_sync_process_loop(az_iot_sm_device_orchestration* client);
#endif // TRANSPORT_MQTT_SYNC

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_SM_DEVICE_ORCHESTRATION_H
