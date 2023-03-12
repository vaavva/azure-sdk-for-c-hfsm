// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>

#include <azure/core/_az_cfg.h>

static az_result _az_iot_subclients_process_event(
    az_event_policy* policy,
    az_event const event)
{
    return AZ_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result _az_mqtt_policy_init(
    _az_mqtt_policy* policy,
    az_mqtt* mqtt,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy)
{
    return AZ_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result _az_iot_subclients_policy_init(
    _az_iot_subclients_policy* policy,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy)
{
  return AZ_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result _az_iot_subclients_policy_add_client(
    _az_iot_subclients_policy* policy,
    _az_iot_subclient const* subclient)
{
  return AZ_NOT_IMPLEMENTED;
}
