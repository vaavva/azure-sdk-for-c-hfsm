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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_provisioning_client.h>
#include <azure/iot/az_iot_hub_client.h>
// TODO: can't be internal:
#include <azure/iot/internal/az_iot_sample_config.h>

// TODO: move to reference include:
static az_iot_provisioning_client provisioning_client;

// TODO: move to PAL include:
static MQTTAsync mqtt_client;


int main()
{
  int rc;
  az_span endpoint;
  az_span id_scope;
  az_span registration_id;

  if (az_failed(rc = read_environment_variables(&endpoint, &id_scope, &registration_id)))
  {
    LOG_ERROR("Failed to read evironment variables: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  if (az_failed(
          rc = az_iot_provisioning_client_init(
              &provisioning_client, endpoint, id_scope, registration_id, NULL)))
  {
    LOG_ERROR("Failed to initialize provisioning client: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  if (az_failed(
          rc = az_iot_provisioning_client_get_client_id(
              &provisioning_client, mqtt_client_id_buffer, sizeof(mqtt_client_id_buffer), NULL)))
  {
    LOG_ERROR("Failed to get MQTT client id: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  // Move to PAL include.
	if ((rc = MQTTAsync_create(
           &mqtt_client,
           (char*)az_span_ptr(endpoint),
           mqtt_client_id_buffer,
           MQTTCLIENT_PERSISTENCE_NONE,
           NULL))   
    != MQTTASYNC_SUCCESS)
	{
    LOG_ERROR("Failed to create MQTT client: MQTTAsync_create return code %d.", rc);
    exit(rc);
	}

  return 0;
}
