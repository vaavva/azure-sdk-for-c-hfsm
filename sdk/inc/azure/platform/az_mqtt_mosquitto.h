// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_MQTT_MOSQUITTO_H
#define _az_MQTT_MOSQUITTO_H

#include "mosquitto.h"
#include <azure/core/internal/az_mqtt_internal.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  az_mqtt_options_common platform_options;
  az_span openssl_engine;
  struct mosquitto* mosquitto_handle;
} az_mqtt_options;

struct az_mqtt
{
  struct {
    az_mqtt_common platform_mqtt;
    az_mqtt_options options;
  } _internal;

  struct mosquitto* mosquitto_handle;
};

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_MOSQUITTO_H
