// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/internal/az_iot_hsm_mqtt.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result mqtt_pal_initialize()
{
    
}

AZ_NODISCARD az_result mqtt_pal_connect_start(/*TODO: params*/)
{
    
}

AZ_NODISCARD az_result mqtt_pal_pub_start()
{

}

AZ_NODISCARD az_result mqtt_pal_sub_start()
{

}

AZ_NODISCARD az_result mqtt_pal_disconnect_start()
{

}

AZ_NODISCARD az_result time_pal_start(int32_t ticks, az_iot_hsm* src)
{
    // No-op: this PAL layer is using the Paho MQTT timeout system to signal timeouts.   
}
