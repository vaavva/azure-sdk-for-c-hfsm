// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_C2D_H
#define _az_IOT_HUB_C2D_H

#include <az_span.h>
#include <az_result.h>
#include "az_iot_hub_connect.h"
#include "az_iot_hub_properties.h"

#include <_az_cfg_prefix.h>

az_result az_iot_hub_c2d_subscribe_topic_filter_get(az_iot_identity* identity, az_span mqtt_topic_filter, az_span* out_mqtt_topic_filter);

typedef struct az_iot_hub_c2d_request {
    az_span properties;
} az_iot_hub_c2d_request;

// TODO: do we want to check the identity in the topic or expose it within the c2d request?
az_result az_iot_c2d_handle(az_span received_topic, az_iot_hub_c2d_request* out_request);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_C2D_H
