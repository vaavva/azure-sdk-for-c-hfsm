// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_mqtt.h
 *
 * @brief This header defines the types and functions your application uses to leverage MQTT pub/sub
 * functionality.
 *
 * @details For more details on Azure IoT MQTT requirements please see
 * https://docs.microsoft.com/azure/iot-hub/iot-hub-mqtt-support.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_HFSM_MQTT_INTERNAL_H
#define _az_HFSM_MQTT_INTERNAL_H

#include <azure/core/az_config.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_hfsm.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Azure MQTT HFSM.
 * @details Derives from az_hfsm.
 *
 */
typedef struct
{
  // Derived from az_policy which is a kind of az_hfsm.
  struct
  {
    az_hfsm_policy policy;

    // Extension point for the implementation.
    // HFSM_DESIGN: We could have different definitions for az_mqtt_impl to support additional
    //              memory reserved for the implementation:
    az_mqtt mqtt;
  } _internal;
} az_hfsm_mqtt_policy;

/**
 * @brief Initializes the MQTT Platform Layer.
 *
 * @param mqtt_hfsm The #az_hfsm MQTT state machine instance.
 * @param parent The IoT client state machine dispatcher that will receive events from this
 *                   machine.
 * @param client_id
 * @param options
 * @return #az_result
 */
AZ_NODISCARD az_result az_mqtt_initialize(
    az_hfsm_mqtt_policy* mqtt_hfsm,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_mqtt mqtt);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_HFSM_MQTT_INTERNAL_H
