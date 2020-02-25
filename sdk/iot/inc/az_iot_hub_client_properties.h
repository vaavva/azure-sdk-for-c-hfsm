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
  } _internal;
} az_iot_hub_properties;

void az_iot_hub_properties_init(az_iot_hub_properties * properties, az_span buffer);
az_result az_iot_hub_properties_append(
    az_iot_hub_properties * properties,
    az_span name,
    az_span value);
az_result az_iot_hub_properties_read(
    az_iot_hub_properties * properties,
    az_span name,
    az_span * out_value);

// TODO: AZ SDKs to followup. az_SDKs common iterator pattern + result types.
typedef struct az_iot_hub_properties_iterator {
  struct {
    az_iot_hub_properties properties;
    uint8_t * current;
  } _internal;
} az_iot_hub_properties_iterator;

az_result az_iot_hub_properties_enumerator_init(
    az_iot_hub_properties_iterator * iterator,
    az_iot_hub_properties properties);
az_result az_iot_hub_properties_enumerator_next(
    az_iot_hub_properties_iterator * iterator,
    az_span * out_key,
    az_span * out_value);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_HUB_PROPERTIES_H
