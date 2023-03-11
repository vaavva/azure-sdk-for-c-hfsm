// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_MQTT_NOTRANSPORT_H
#define _az_MQTT_NOTRANSPORT_H

#include <azure/core/internal/az_mqtt_internal.h>

#include <azure/core/_az_cfg_prefix.h>

struct az_mqtt
{
  az_mqtt_common platform_mqtt;
};

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_NOTRANSPORT_H
