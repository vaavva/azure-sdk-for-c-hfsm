// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_sm_provisioning_client.h
 *
 * @brief Definition for the Azure Device Provisioning client state machine.
 * @remark The Device Provisioning MQTT protocol is described at
 * https://docs.microsoft.com/azure/iot-dps/iot-dps-mqtt-support
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_SM_PROVISIONING_CLIENT_H
#define _az_IOT_SM_PROVISIONING_CLIENT_H

#include <azure/core/az_mqtt.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/internal/az_iot_provisioning_hfsm.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  bool _unused;
} az_iot_sm_provisioning_client_options;

typedef struct
{
  struct
  {
    az_hfsm_iot_provisioning_policy policy;
  } _internal;
} az_iot_sm_provisioning_client;

AZ_NODISCARD az_iot_sm_provisioning_client_options az_iot_sm_provisioning_client_options_default();

AZ_NODISCARD az_result az_iot_sm_provisioning_client_initialize(
    az_iot_sm_provisioning_client* client,
    az_iot_provisioning_client* codec_client,
    az_hfsm_iot_auth_type auth_type,
    az_hfsm_iot_auth auth);

AZ_NODISCARD az_result az_iot_sm_provisioning_client_destroy(az_iot_sm_provisioning_client* client);

typedef az_result (*az_iot_sm_provisioning_client_register_status_callback)(
    az_iot_sm_provisioning_client* client);

typedef az_result (*az_iot_sm_provisioning_client_register_result_callback)(
    az_iot_sm_provisioning_client* client);

AZ_NODISCARD az_result az_iot_sm_provisioning_client_register(
    az_iot_sm_provisioning_client* client,
    az_iot_sm_provisioning_client_register_status_callback* status_callback,
    az_iot_sm_provisioning_client_register_result_callback* result_callback);

AZ_NODISCARD az_result
az_iot_sm_provisioning_client_register_abort(az_iot_sm_provisioning_client* client);

AZ_NODISCARD az_result
az_iot_sm_provisioning_client_register_get_status(az_iot_sm_provisioning_client* client);

AZ_NODISCARD az_result
az_iot_sm_provisioning_client_register_get_result(az_iot_sm_provisioning_client* client);

#ifdef TRANSPORT_MQTT_SYNC
AZ_NODISCARD az_result
az_iot_sm_provisioning_client_sync_process_loop(az_iot_sm_provisioning_client* client);
#endif // TRANSPORT_MQTT_SYNC

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_SM_PROVISIONING_CLIENT_H
