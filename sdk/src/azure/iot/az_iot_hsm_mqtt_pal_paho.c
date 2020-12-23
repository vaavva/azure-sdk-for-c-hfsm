// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTAsync.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/internal/az_iot_hsm_mqtt.h>

#include <azure/core/_az_cfg.h>

MQTTAsync mqtt_client;

AZ_NODISCARD az_result mqtt_pal_initialize(az_iot_hsm* hsm)
{

}

AZ_NODISCARD az_result mqtt_pal_connect_start(az_iot_hsm* hsm, const char* hostname, int port, const char* client_id) 
{
  int rc = 0;
  (void)port; // The hostname contains a full enpoint name in case of the Paho PAL.

  if ((rc = MQTTAsync_create(
           &mqtt_client,
           hostname,
           client_id,
           MQTTCLIENT_PERSISTENCE_NONE,
           NULL))
      != MQTTASYNC_SUCCESS)
  {
    LOG_ERROR("Failed to create MQTT client: MQTTAsync_create return code %d.", rc);
    
  }


}

AZ_NODISCARD az_result mqtt_pal_pub_start() {}

AZ_NODISCARD az_result mqtt_pal_sub_start() {}

AZ_NODISCARD az_result mqtt_pal_disconnect_start() {}

AZ_NODISCARD az_result time_pal_start(int32_t ticks, az_iot_hsm* src)
{
  // No-op: this PAL layer is using the Paho MQTT timeout system to signal timeouts.
}
