// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_MQTT_PAHO_H
#define _az_MQTT_PAHO_H

#ifdef _MSC_VER
#pragma warning(push)
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <azure/core/internal/az_mqtt_internal.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  az_mqtt_options_common platform_options;
} az_mqtt_options;

struct az_mqtt
{
  struct {
    az_mqtt_common platform_mqtt;
    az_mqtt_options options;
  } _internal;

  MQTTClient paho_handle;

  // Last message and topic lifetime management.
  MQTTClient_message* last_message;
  char* last_topic;
};

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_PAHO_H
