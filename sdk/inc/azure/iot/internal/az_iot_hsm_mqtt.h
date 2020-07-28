// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_hsm_mqtt.h
 *
 * @brief Azure IoT MQTT State Machine
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_HSM_MQTT_INTERNAL_H
#define _az_IOT_HSM_MQTT_INTERNAL_H

#include <azure/iot/internal/az_iot_hsm.h>

#include <azure/core/_az_cfg_prefix.h>

extern az_iot_hsm az_iot_hsm_mqtt;

typedef enum
{
  CONNECT_REQ = _az_IOT_HSM_EVENT(1),
  CONNECT_RSP = _az_IOT_HSM_EVENT(2),
  MQTT_PUB_REQ = _az_IOT_HSM_EVENT(3),
  MQTT_PUB_RSP = _az_IOT_HSM_EVENT(4),
  MQTT_SUB_REQ = _az_IOT_HSM_EVENT(5),
  MQTT_SUB_RSP = _az_IOT_HSM_EVENT(6),
  DISCONNECT_REQ = _az_IOT_HSM_EVENT(7),
  ON_MQTT_PAL_CONNECTED = _az_IOT_HSM_EVENT(8),
  ON_MQTT_PAL_RECV = _az_IOT_HSM_EVENT(9),
  ON_MQTT_PAL_PUB_ACK = _az_IOT_HSM_EVENT(10),
  ON_MQTT_PAL_SUB_ACK = _az_IOT_HSM_EVENT(11),
  ON_MQTT_PAL_DISCONNECTED = _az_IOT_HSM_EVENT(12),
} az_iot_hsm_mqtt_event_type;

typedef struct az_iot_hsm_mqtt_event_data
{
  char* topic;
  void* data;
} az_iot_hsm_mqtt_event_data;

#include <azure/core/_az_cfg_suffix.h>

#endif //!_az_IOT_HSM_MQTT_INTERNAL_H
