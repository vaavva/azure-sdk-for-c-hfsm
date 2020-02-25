// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_CLIENT_TELEMETRY_H
#define _az_IOT_HUB_CLIENT_TELEMETRY_H

#include "az_iot_hub_client.h"
#include "az_iot_hub_client_properties.h"
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

az_result az_iot_client_telemetry_publish_topic_get(
    az_iot_hub_client const * client,
    az_iot_hub_properties const * properties,
    az_span mqtt_topic,
    az_span * out_mqtt_topic);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_CLIENT_TELEMETRY_H
