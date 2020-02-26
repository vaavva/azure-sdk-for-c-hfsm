// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_HUB_PROPERTIES_H
#define _az_IOT_HUB_PROPERTIES_H

#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_properties {
  struct {
    az_span properties;
    uint8_t * current_property;
  } _internal;
} az_iot_hub_properties;

void az_iot_hub_properties_init(az_iot_hub_properties * properties, az_span buffer);
az_result az_iot_hub_properties_append(
    az_iot_hub_properties * properties,
    az_span name,
    az_span value);

az_result az_iot_hub_properties_find(
    az_iot_hub_properties * properties,
    az_span name,
    az_span * out_value);

az_result az_iot_hub_properties_next(
    az_iot_hub_properties * properties,
    az_pair * out);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_PROPERTIES_H
