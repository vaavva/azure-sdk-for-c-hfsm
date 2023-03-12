// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_mqtt_policy.h>

#include <azure/core/_az_cfg.h>

static az_result _az_mqtt_policy_process_outbound_event(
    az_event_policy* policy,
    az_event const event)
{
    _az_mqtt_policy* me = (_az_mqtt_policy*)policy;
    switch(event.type)
    {
        case AZ_MQTT_EVENT_CONNECT_REQ:
        case AZ_MQTT_EVENT_DISCONNECT_REQ:
        case AZ_MQTT_EVENT_PUB_REQ:
        case AZ_MQTT_EVENT_SUB_REQ:
        default:
            az_platform_critical_error();
    }

    return AZ_ERROR_NOT_IMPLEMENTED;
}

static az_result _az_mqtt_policy_process_inbound_event(
    az_event_policy* policy,
    az_event const event)
{
    _az_mqtt_policy* me = (_az_mqtt_policy*)policy;
    switch(event.type)
    {
        case AZ_MQTT_EVENT_CONNECT_RSP:
        case AZ_MQTT_EVENT_DISCONNECT_RSP:
        case AZ_MQTT_EVENT_PUB_RECV_IND:
        case AZ_MQTT_EVENT_PUBACK_RSP:
        case AZ_MQTT_EVENT_SUBACK_RSP:
        default:
            az_platform_critical_error();
    }

    return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result _az_mqtt_policy_init(
    _az_mqtt_policy* policy,
    az_mqtt* mqtt,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy)
{
    return AZ_ERROR_NOT_IMPLEMENTED;
}
