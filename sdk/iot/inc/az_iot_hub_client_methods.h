// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_CLIENT_METHODS_H
#define _az_IOT_CLIENT_METHODS_H

#include "az_iot_hub_client.h"
#include "az_iot_status.h"
#include <az_result.h>
#include <az_span.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

az_result az_iot_hub_client_methods_subscribe_topic_filter_get(
    az_iot_hub_client const * client,
    az_span mqtt_topic_filter,
    az_span * out_mqtt_topic_filter);

typedef struct az_iot_hub_method_request {
  az_span request_id;
  az_span name;
} az_iot_method_response;

az_result az_iot_hub_client_methods_topic_parse(
    az_iot_hub_client const * client,
    az_span recevied_topic,
    az_iot_hub_method_request * out_request);

az_result az_iot_hub_client_methods_publish_topic_get(
    az_iot_hub_client const * client,
    az_span request_id,
    int status,
    az_span mqtt_topic,
    az_span * out_mqtt_topic);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_CLIENT_METHODS_H
