// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_PROVISIONING_SAS_H
#define _az_IOT_PROVISIONING_SAS_H

#include <az_span.h>
#include <az_result.h>
#include <time.h>
#include "az_iot_mqtt.h"
#include "az_iot_provisioning_client.h"

#include <_az_cfg_prefix.h>

// Provisioning
typedef struct az_iot_provisioning_sas {
    struct {
        az_span signature;
        az_span signature_time;
        az_span key_name; // Can be empty.
        uint32_t last_unix_time;
    } _internal;
} az_iot_provisioning_sas;

az_result az_iot_provisioning_sas_init(az_iot_provisioning_sas* sas, az_iot_provisioning_client* client, az_span id_scope, az_span key_name);
az_result az_iot_provisioning_sas_update_signature(az_iot_provisioning_sas* sas, uint32_t unix_time, az_span to_sign);
az_result az_iot_provisioning_sas_update_password(az_iot_provisioning_sas* sas, az_span base64_hmac256_signature, az_iot_mqtt_connect* mqtt_connect);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_PROVISIONING_SAS_H
