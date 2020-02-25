// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_TWIN_H
#define _az_IOT_TWIN_H

#include "az_iot_hub_client.h"
#include "az_iot_status.h"
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

az_result az_iot_hub_client_twin_response_subscribe_topic_filter_get(
    az_iot_hub_client const * client,
    az_span mqtt_topic_filter,
    az_span * out_mqtt_topic_filter);

az_result az_iot_hub_client_twin_patch_subscribe_topic_filter_get(
    az_iot_hub_client const * client,
    az_span mqtt_topic_filter,
    az_span * out_mqtt_topic_filter);

typedef struct az_iot_hub_twin_response {
  enum az_iot_status status;
  az_span request_id;
  az_span version; // NULL when received as a response for twin_get.
} az_iot_hub_twin_response;

// Handles both responses and twin patches.
az_result az_iot_client_twin_topic_parse(
    az_iot_hub_client const * client,
    az_span received_topic,
    az_iot_hub_twin_response * out_twin_response);

// Requires an empty MQTT payload.
// TODO: better name?
az_result az_iot_twin_get_publish_topic_get(
    az_iot_hub_client const * client,
    az_span request_id,
    az_span mqtt_topic,
    az_span * out_mqtt_topic);

// MQTT payload contains a JSON document formatted according to the Twin specification.
az_result az_iot_twin_patch_publish_topic_get(
    az_iot_hub_client const * client,
    az_span request_id,
    az_span mqtt_topic,
    az_span * out_mqtt_topic);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_TWIN_H
