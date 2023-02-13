// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 */

#include "mosquitto.h"
#include <azure/core/az_mqtt.h>

typedef struct
{
  az_mqtt mqtt;
  struct mosquitto* mosquitto_handle;
} az_mqtt_mosquitto;

AZ_NODISCARD az_result az_mqtt_mosquitto_init(
    az_mqtt_mosquitto* mqtt,
    struct mosquitto* mosquitto_handle,
    az_mqtt_options const* options);
