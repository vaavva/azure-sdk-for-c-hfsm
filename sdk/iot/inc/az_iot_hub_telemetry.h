// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_TELEMETRY_H
#define _az_IOT_HUB_TELEMETRY_H

#include <az_span.h>
#include <az_result.h>
#include "az_iot_mqtt.h"
#include "az_iot_hub_client.h"
#include "az_iot_hub_properties.h"

#include <_az_cfg_prefix.h>

az_result az_iot_telemetry_create(az_iot_hub_client* client, az_iot_hub_properties* properties, az_iot_mqtt_publish *mqtt_publish);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_TELEMETRY_H
