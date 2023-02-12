// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_mosquitto.c
 *
 * @brief Contains the az_mqtt.h interface implementation with Mosquitto MQTT
 * (https://github.com/eclipse/mosquitto).
 *
 * @remarks Both a non-blocking I/O (default) as well as a blocking I/O implementations are
 * available. To enable the blocking mode, set TRANSPORT_MQTT_SYNC.
 *
 * @note The Mosquitto Lib documentation is available at:
 * https://mosquitto.org/api/files/mosquitto-h.html
 */

#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>
#include <azure/iot/az_iot_common.h>

#include <paho.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_mqtt_options az_mqtt_options_default()
{
}

AZ_NODISCARD az_result az_mqtt_init(az_mqtt* mqtt, az_mqtt_options const* options)
{

}

AZ_NODISCARD az_result
az_mqtt_outbound_connect(az_mqtt* mqtt, az_context* context, az_mqtt_connect_data* connect_data)
{

}

AZ_NODISCARD az_result
az_mqtt_outbound_sub(az_mqtt* mqtt, az_context* context, az_mqtt_sub_data* sub_data)
{

}

AZ_NODISCARD az_result
az_mqtt_outbound_pub(az_mqtt* mqtt, az_context* context, az_mqtt_pub_data* pub_data)
{

}

AZ_NODISCARD az_result az_mqtt_outbound_disconnect(az_mqtt* mqtt, az_context* context)
{

}

AZ_NODISCARD az_result az_mqtt_wait_for_event(az_mqtt* mqtt, int32_t timeout)
{

}
