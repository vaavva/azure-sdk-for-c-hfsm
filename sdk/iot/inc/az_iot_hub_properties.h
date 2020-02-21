// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_PROPERTIES_H
#define _az_IOT_HUB_PROPERTIES_H

#include <az_span.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

az_result az_iot_hub_properties_add(az_span properties, az_span name, az_span value);
az_result az_iot_hub_properties_read(az_span properties, az_span* out_name, az_span* out_value);
az_result az_iot_hub_properties_remove(az_span properties, az_span name);

// TODO: az_SDKs common iterator pattern + result types.
typedef struct az_iot_hub_properties_iterator {
    struct {
        az_span properties;
        uint8_t* current;
    } _internal;
} az_iot_hub_properties_iterator;

az_result az_iot_hub_properties_enumerator_init(az_iot_hub_properties_iterator* iterator,  az_span properties);
az_result az_iot_hub_properties_enumerator_next(az_iot_hub_properties_iterator* iterator,  az_span* out_key, az_span* out_value);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_PROPERTIES_H
