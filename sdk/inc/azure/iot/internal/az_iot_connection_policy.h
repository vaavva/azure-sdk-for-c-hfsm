// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_connection.h
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

#ifndef _az_IOT_CONNECTION_POLICY
#define _az_IOT_CONNECTION_POLICY

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_hfsm.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  _az_hfsm policy;
  az_iot_connection * connection;
} _az_iot_connection_policy;

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_CONNECTION_POLICY
