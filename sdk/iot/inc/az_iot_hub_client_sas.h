// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_CLIENT_SAS_H
#define _az_IOT_CLIENT_SAS_H

#include "az_iot_hub_client.h"
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg_prefix.h>
    
az_result az_iot_hub_client_sas_signature_get(
    az_iot_hub_client const * client,
    uint32_t token_expiration_unix_time,
    az_span signature,
    az_span * out_signature);

// TODO: mqtt_password_get vs sas_password_get vs _password_get.
az_result az_iot_hub_client_password_get(
    az_iot_hub_client const * client,
    az_span base64_hmac_sha256_signature,
    az_span key_name,
    az_span mqtt_password,
    az_span * out_mqtt_password);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_CLIENT_SAS_H
