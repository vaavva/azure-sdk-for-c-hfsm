// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_PROVISIONING_CONNECT_SAS_H
#define _az_IOT_PROVISIONING_CONNECT_SAS_H

#include <az_span.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

// Provisioning
typedef struct az_iot_provisioning_sas {
    struct {
        az_span signature;
        az_span signature_time; // Within the signature buffer.
        az_span key_name; // Can be empty.
        uint32_t last_token_expiration_unix_time;
    } _internal;
} az_iot_provisioning_sas;

// Note: creates the "audience" portion.
az_result az_iot_provisioning_sas_init(az_iot_provisioning_sas* sas, az_span id_scope, az_span registration_id, az_span optional_key_name, az_span signature_buffer);
az_result az_iot_provisioning_sas_update_signature(az_iot_provisioning_sas* sas, uint32_t token_expiration_unix_time, az_span to_hmac_sha256_sign);
az_result az_iot_provisioning_sas_update_password(az_iot_provisioning_sas* sas, az_span base64_hmac_sha256_signature, az_span mqtt_password, az_span* out_mqtt_password);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_PROVISIONING_CONNECT_SAS_H
