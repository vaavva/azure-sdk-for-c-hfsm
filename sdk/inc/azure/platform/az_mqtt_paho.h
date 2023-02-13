// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 */

#include <azure/core/az_mqtt.h>
#ifdef _MSC_VER
#pragma warning(push)
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

typedef struct
{
  az_mqtt mqtt;

  MQTTClient paho_handle;

  // Last message and topic lifetime management.
  MQTTClient_message* last_message;
  char* last_topic;
} az_mqtt_paho;

AZ_NODISCARD az_result
az_mqtt_paho_init(az_mqtt* mqtt, MQTTClient* paho_handle, az_mqtt_options const* options);
