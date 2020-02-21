// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_METHODS_H
#define _az_IOT_METHODS_H

#include <stdint.h>
#include <az_span.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

az_result az_iot_hub_methods_subscribe_topic_filter_get(az_span mqtt_topic_filter, az_span* out_mqtt_topic_filter);

typedef struct az_iot_method_request {
    az_span request_id;
    az_span name;
} az_iot_method_response;

az_result az_iot_hub_methods_handle(az_span topic, az_iot_method_request* out_request);

typedef struct az_iot_method_response {
    az_span request_id;
    uint8_t status;
} az_iot_method_response;

az_result az_iot_hub_methods_publish_topic_get(az_iot_method_response* response, az_span mqtt_topic, az_span *out_mqtt_topic);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_METHODS_H
