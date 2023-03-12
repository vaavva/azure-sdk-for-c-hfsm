/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Mosquitto async callback
 *
 */

#include <az_log_listener.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/az_core.h>
#include <azure/az_iot.h>
#include <azure/core/az_log.h>

static const az_span dps_endpoint
    = AZ_SPAN_LITERAL_FROM_STR("global.azure-devices-provisioning.net");
static const az_span id_scope = AZ_SPAN_LITERAL_FROM_STR("0ne00003E26");
static const az_span device_id = AZ_SPAN_LITERAL_FROM_STR("dev1-ecc");
static const az_span ca_path = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/rsa_baltimore_ca.pem");

static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/dev1-ecc_cert.pem");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/dev1-ecc_key.pem");

static const az_span cert_path2 = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/dev1-ecc_cert.pem");
static const az_span key_path2 = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/dev1-ecc_key.pem");

static char client_id_buffer[64];
static char username_buffer[128];

static char topic_buffer[128];
static char payload_buffer[256];

volatile bool connected = false;

void az_platform_critical_error()
{
  printf(LOG_APP "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

az_result iot_callback(az_iot_connection* client, az_event event)
{
  switch (event.type)
  {
    case AZ_MQTT_EVENT_CONNECT_RSP:
    {
      connected = true;
      az_mqtt_connack_data* connack_data = (az_mqtt_connack_data*)event.data;
      printf(LOG_APP "[%p] CONNACK: %d\n", client, connack_data->connack_reason);
      break;
    }

    case AZ_MQTT_EVENT_DISCONNECT_RSP:
    {
      connected = false;
      printf(LOG_APP "[%p] DISCONNECTED\n", client);
      break;
    }

    default:
      // TODO:
      break;
  }

  return AZ_OK;
}

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  /* Required before calling other mosquitto functions */
  if (mosquitto_lib_init() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to initialize MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Using MosquittoLib %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  az_mqtt mqtt;
  az_mqtt_options mqtt_options = az_mqtt_options_default();
  mqtt_options.platform_options.certificate_authority_trusted_roots = ca_path;

  LOG_AND_EXIT_IF_FAILED(az_mqtt_init(&mqtt, &mqtt_options));

  az_iot_connection iot_connection;

  az_credential_x509 primary_credential = (az_credential_x509){
    .cert = cert_path1,
    .key = key_path1,
    .key_type = AZ_CREDENTIALS_X509_KEY_MEMORY,
  };

  az_credential_x509 secondary_credential = (az_credential_x509){
    .cert = cert_path2,
    .key = key_path2,
    .key_type = AZ_CREDENTIALS_X509_KEY_MEMORY,
  };

  az_iot_connection_options connection_options = az_iot_connection_options_default();
  connection_options.connection_management = true;
  connection_options.client_id_buffer = AZ_SPAN_FROM_BUFFER(client_id_buffer);
  connection_options.username_buffer = AZ_SPAN_FROM_BUFFER(username_buffer);
  connection_options.password_buffer = AZ_SPAN_EMPTY;
  connection_options.primary_credential = &primary_credential;
  connection_options.secondary_credential = &secondary_credential;

  az_context connection_context = az_context_create_with_expiration(
      &az_context_application, az_context_get_expiration(&az_context_application));

  LOG_AND_EXIT_IF_FAILED(az_iot_connection_init(
      &iot_connection, &connection_context, &mqtt, iot_callback, &connection_options));

  az_iot_provisioning_client prov_client;
  LOG_AND_EXIT_IF_FAILED(az_iot_provisioning_client_init(
      &prov_client, &iot_connection, dps_endpoint, id_scope, device_id, NULL));

  LOG_AND_EXIT_IF_FAILED(az_iot_connection_open(&iot_connection));

  az_context register_context = az_context_create_with_expiration(&connection_context, 30 * 1000);

  az_iot_provisioning_register_data register_data = {
    .topic_buffer = AZ_SPAN_FROM_BUFFER(topic_buffer),
    .payload_buffer = AZ_SPAN_FROM_BUFFER(payload_buffer),
  };

  // TODO : LOG_AND_EXIT_IF_FAILED(
  // TODO :     az_iot_provisioning_client_register(&prov_client, &register_context,
  // &register_data));

  for (int i = 15; i > 0; i--)
  {
    LOG_AND_EXIT_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Waiting %ds        \r", i);
    fflush(stdout);
  }

  LOG_AND_EXIT_IF_FAILED(az_iot_connection_close(&iot_connection));

  for (int i = 15; connected && i > 0; i--)
  {
    LOG_AND_EXIT_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Waiting for disconnect %ds        \r", i);
    fflush(stdout);
  }

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                              \n");
  return 0;
}
