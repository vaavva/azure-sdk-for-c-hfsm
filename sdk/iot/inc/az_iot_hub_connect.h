// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_CLIENT_H
#define _az_IOT_HUB_CLIENT_H

#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct az_iot_identity 
{
    struct 
    {
        az_span device_id;
        az_span module_id; // can be null if this is a device.
    } _internal;
} az_iot_identity;

az_result az_iot_hub_idenity_init(az_iot_identity *identity, az_span device_id, az_span module_id);
az_result az_iot_hub_user_name_get(az_span iot_hub, az_iot_identity identity, az_span user_agent, az_span mqtt_user_name, az_span* out_mqtt_user_name);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_CLIENT_H
