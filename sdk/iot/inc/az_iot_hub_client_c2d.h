// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_CLIENT_C2D_H
#define _az_IOT_HUB_CLIENT_C2D_H

#include "az_iot_hub_client.h"
#include "az_iot_hub_client_properties.h"
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

az_result az_iot_hub_client_c2d_subscribe_topic_filter_get(
    az_iot_hub_client const * client,
    az_span mqtt_topic_filter,
    az_span * out_mqtt_topic_filter);

typedef struct az_iot_hub_client_c2d_request {
  az_iot_hub_properties properties;
} az_iot_hub_client_c2d_request;

// May return AZ_IOT_FAIL_TO_PARSE.
az_result az_iot_hub_client_c2d_topic_parse(
    az_iot_hub_client const * client,
    az_span received_topic,
    az_iot_hub_client_c2d_request * out_request);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_CLIENT_C2D_H
