// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_CONNECT_SAS_H
#define _az_IOT_CONNECT_SAS_H

#include <az_span.h>
#include <az_result.h>
#include "az_iot_hub_connect.h"

#include <_az_cfg_prefix.h>

typedef struct az_iot_hub_connect_sas {
    struct {
        az_span signature;
        az_span signature_time; // Within the signature buffer.
        az_span key_name; // Can be empty.
        uint32_t last_unix_time;
    } _internal;
} az_iot_hub_sas;

// TODO: passing in the initial buffer for signature:
// Note: creates the "audience" portion.
az_result az_iot_hub_sas_init(az_iot_hub_sas* sas, az_iot_identity* identity, az_span hub_name, az_span key_name, az_span signature_buffer);
az_result az_iot_hub_sas_update_signature(az_iot_hub_sas* sas, uint32_t token_expiration_unix_time, az_span to_hmac_sha256_sign);
az_result az_iot_hub_sas_update_password(az_iot_hub_sas* sas, az_span base64_hmac_sha256_signature, az_span mqtt_password, az_span* out_mqtt_password);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_CONNECT_SAS_H
