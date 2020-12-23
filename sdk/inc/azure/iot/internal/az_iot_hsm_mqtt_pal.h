// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_hsm_mqtt_pal.h
 *
 * @brief Azure IoT MQTT Platform Adaptation Layer
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_HSM_MQTT_PAL_INTERNAL_H
#define _az_IOT_HSM_MQTT_PAL_INTERNAL_H

#include <azure/core/az_result.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/internal/az_iot_hsm_mqtt.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

AZ_NODISCARD az_result mqtt_pal_initialize();

AZ_NODISCARD az_result mqtt_pal_connect_start(/*TODO: params*/);
AZ_INLINE az_result mqtt_pal_connected(bool isError, bool isTimeout, void* data)
{
  az_iot_hsm_event evt = { ON_MQTT_PAL_CONNECTED, data };
  if (isError)
  {
    evt.type = AZ_IOT_HSM_ERROR;
  }
  else if (isTimeout)
  {
    evt.type = AZ_IOT_HSM_TIMEOUT;
  }

  return az_iot_hsm_post_sync(&az_iot_hsm_mqtt, evt);
}

AZ_INLINE az_result mqtt_pal_signal_error(void* data)
{
  az_iot_hsm_event evt = { AZ_IOT_HSM_ERROR, data };
  return az_iot_hsm_post_sync(&az_iot_hsm_mqtt, evt);
}

AZ_NODISCARD az_result mqtt_pal_pub_start();
AZ_INLINE az_result mqtt_pal_signal_pub_ack_received(void* data)
{
  az_iot_hsm_event evt = { ON_MQTT_PAL_PUB_ACK, data };
  return az_iot_hsm_post_sync(&az_iot_hsm_mqtt, evt);
}

AZ_NODISCARD az_result mqtt_pal_sub_start();
AZ_INLINE az_result mqtt_pal_signal_sub_ack_received(void* data)
{
  az_iot_hsm_event evt = { ON_MQTT_PAL_SUB_ACK, data };
  return az_iot_hsm_post_sync(&az_iot_hsm_mqtt, evt);
}

AZ_NODISCARD az_result mqtt_pal_disconnect_start();
AZ_INLINE az_result mqtt_pal_disconnected(void* data)
{
  az_iot_hsm_event evt = { ON_MQTT_PAL_DISCONNECTED, data };
  return az_iot_hsm_post_sync(&az_iot_hsm_mqtt, evt);
}

#endif //!_az_IOT_HSM_MQTT_PAL_INTERNAL_H
