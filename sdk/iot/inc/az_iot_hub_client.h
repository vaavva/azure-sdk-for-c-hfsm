// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_CLIENT_H
#define _az_IOT_HUB_CLIENT_H

#include <az_span.h>
#include "az_iot_mqtt.h"

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_client {
    struct {
        az_span device_id;
        az_span module_id;
    } _internal;
} az_provisioning_client;

az_result az_iot_hub_client_device_init(az_iot_hub_client* client, az_span iot_hub, az_span device_id, az_iot_mqtt_connect* mqtt_connect);
az_result az_iot_hub_client_module_init(az_iot_hub_client* client, az_span iot_hub, az_span device_id, az_span module_id, az_iot_mqtt_connect* mqtt_connect);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_CLIENT_H
