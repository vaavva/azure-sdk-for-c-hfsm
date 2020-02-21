// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_PROVISIONING_CLIENT_H
#define _az_IOT_PROVISIONING_CLIENT_H

#include <az_span.h>
#include <az_result.h>

#include <_az_cfg_prefix.h>

az_result az_iot_provisioning_user_name_get(az_span id_scope, az_span registration_id, az_span user_agent, az_span mqtt_user_name, az_span* out_mqtt_user_name);

#include <_az_cfg_suffix.h>

#endif //!_az_IOT_PROVISIONING_CLIENT_H
