// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Azure IoT public headers.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_H
#define _az_IOT_H

#include <azure/iot/az_iot_common.h>
#include <azure/iot/az_iot_hub_client.h>
#include <azure/iot/az_iot_hub_client_properties.h>
#include <azure/iot/az_iot_provisioning_client.h>
#include <azure/iot/az_mqtt_rpc_server.h>

// HFSM_DESIGN: The following are required for the new stateful clients.
#include <azure/core/az_event.h>
#include <azure/core/az_mqtt.h>
#include <azure/iot/az_iot_connection.h>


#endif // _az_IOT_CORE_H
