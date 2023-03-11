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
#include <azure/core/az_context.h>
#include <azure/core/az_credentials_x509.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/internal/az_event_pipeline.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

struct az_iot_connection_policy
{
  struct
  {
    az_event_policy policy;

  } _internal;
};

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_CONNECTION_POLICY
