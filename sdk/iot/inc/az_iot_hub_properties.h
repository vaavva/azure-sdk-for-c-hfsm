// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_PROPERTIES_H
#define _az_IOT_HUB_PROPERTIES_H

#include <az_span.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_properties {
    struct {
        az_span property_string;   // "URIENCODED(name1=val1&name2=val2&...)"
    } _internal;
} az_iot_hub_properties;

az_result az_iot_hub_properties_add(az_iot_hub_properties *properties, az_span name, az_span value);
az_result az_iot_hub_properties_read(az_iot_hub_properties *properties, az_span name, az_span value);
az_result az_iot_hub_properties_remove(az_iot_hub_properties *properties, az_span name);

// TODO: az_SDKs common iterator pattern + result types.
typedef struct az_iot_hub_properties_iterator {
    struct {
        az_iot_hub_properties properties;
        char* current;
    } _internal;
} az_iot_hub_properties_iterator;

az_result az_iot_hub_properties_enumerator_init(az_iot_hub_properties_iterator* iterator,  az_iot_hub_properties* properties);
az_result az_iot_hub_properties_enumerator_next(az_iot_hub_properties_iterator* iterator,  az_span* key, az_span* value);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_PROPERTIES_H

