// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_TWIN_H
#define _az_IOT_TWIN_H

#include <az_span.h>
#include <az_result.h>
#include "az_iot_mqtt.h"
#include "az_iot_hub_client.h"

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_twin_response {
    uint8_t status;
    az_span request_id;
    az_span version; // NULL when received as a response for twin_get.
} az_iot_hub_twin_response;

az_result az_iot_hub_twin_response_subscribe_topic_get(az_iot_hub_client* client, az_iot_topic* mqtt_topic_filter);
az_result az_iot_hub_twin_patch_subscribe_topic_get(az_iot_hub_client* client, az_iot_topic* mqtt_topic_filter);

az_result az_iot_twin_get_create(az_iot_hub_client* client, az_span request_id, az_iot_mqtt_publish *mqtt_publish);

az_result az_iot_twin_patch_create(az_iot_hub_client* client, az_span request_id, az_iot_mqtt_publish *mqtt_publish);

// Handles both responses and twin patches.
az_result az_iot_twin_handle(az_iot_hub_client* client, az_iot_mqtt_publish* pub_received, az_iot_hub_twin_response* twin_response);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_TWIN_H
