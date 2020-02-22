// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_PROVISIONING_REGISTER_H
#define _az_IOT_PROVISIONING_REGISTER_H

#include <az_span.h>
#include <az_result.h>
#include "az_iot_status.h"

#include <_az_cfg_prefix.h>

az_result az_iot_provisioning_register_subscribe_topic_filter_get(az_span mqtt_topic_filter, az_span* out_mqtt_topic_filter);

typedef struct az_iot_provisioning_register_registration_state
{
    az_span assigned_hub;
    az_span device_id;
    az_span json_payload;
    az_span error_code;
    az_span error_message;
} az_iot_provisioning_register_registration_state;

typedef struct az_iot_provisioning_register_response
{
    enum az_iot_status status;
    az_span request_id;
    az_span state; // "unassigned", "assigning", "assigned". TODO: #defines?
    uint32_t retry_after;
    az_iot_provisioning_register_registration_state registration_state;
} az_iot_provisioning_register_response;

az_result az_iot_provisioning_register_handle(az_span received_topic, az_span received_payload, az_iot_provisioning_register_response* out_response);

// Note: payload may contain JSON-encoded data.
az_result az_iot_provisioning_register_publish_topic_get(az_span registration_id, az_span mqtt_topic, az_span *out_mqtt_topic);

az_result az_iot_provisioning_get_operation_status_publish_topic_get(az_span request_id, az_span mqtt_topic, az_span *out_mqtt_topic);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_PROVISIONING_REGISTER_H
