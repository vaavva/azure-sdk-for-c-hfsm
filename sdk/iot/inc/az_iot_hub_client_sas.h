// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_CLIENT_SAS_H
#define _az_IOT_CLIENT_SAS_H

#include "az_iot_hub_client.h"
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_client_sas {
  struct {
    az_iot_hub_client const * client; // TODO: should this be a copy?
    az_span signature;
    az_span signature_time; // Within the signature buffer.
    az_span key_name; // Can be empty.
    uint32_t token_unix_time;
  } _internal;
} az_iot_hub_client_sas;

// Note: creates the "audience" portion.
void az_iot_hub_client_sas_init(
    az_iot_hub_client_sas const * sas,
    az_iot_hub_client const * client,
    az_span signature_buffer,
    az_span key_name);
az_result az_iot_hub_client_sas_signature_get(
    az_iot_hub_client_sas const * sas,
    uint32_t current_unix_time,
    uint32_t token_expiration_unix_time,
    az_span * out_signature);

// TODO: client namespace to match user_name_get:
az_result az_iot_hub_client_password_get(
    az_iot_hub_client_sas const * sas,
    az_span base64_hmac_sha256_signature,
    az_span mqtt_password,
    az_span * out_mqtt_password);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_CLIENT_SAS_H
