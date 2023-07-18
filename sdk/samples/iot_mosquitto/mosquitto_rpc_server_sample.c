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

static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("/home/vaavva/repos/MqttApplicationSamples/scenarios/command/vehicle03.pem");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("/home/vaavva/repos/MqttApplicationSamples/scenarios/command/vehicle03.key");

static char client_id_buffer[64];
static char username_buffer[128];

static char correlation_id_buffer[37];
static char response_topic_buffer[256];
static char sub_topic_buffer[256];

static az_iot_connection iot_connection;
static az_context connection_context;

static az_mqtt_rpc_server rpc_server;

volatile bool connected = false;

static az_context rpc_server_context;
static az_mqtt_rpc_server_options rpc_server_options;

void az_platform_critical_error()
{
  printf(LOG_APP "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

az_iot_status execute_command(az_mqtt_rpc_server_pending_command* command_data)
{
  // for now, just print details from the command
  printf(LOG_APP "Executing command to return to: %s\n", az_span_ptr(command_data->response_topic));

  return AZ_IOT_STATUS_OK;
}

az_result iot_callback(az_iot_connection* client, az_event event)
{
  printf(LOG_APP "[APP/callback] %d\n", event.type);
  switch (event.type)
  {
    case AZ_MQTT_EVENT_CONNECT_RSP:
    {
      connected = true;
      az_mqtt_connack_data* connack_data = (az_mqtt_connack_data*)event.data;
      printf(LOG_APP "[%p] CONNACK: %d\n", client, connack_data->connack_reason);

      rpc_server_context = az_context_create_with_expiration(&connection_context, 30 * 1000);
      rpc_server_options = (az_mqtt_rpc_server_options){
        .sub_topic = AZ_SPAN_FROM_BUFFER(sub_topic_buffer),
        .pending_command = (az_mqtt_rpc_server_pending_command){
          .correlation_id = AZ_SPAN_FROM_BUFFER(correlation_id_buffer),
          .response_topic = AZ_SPAN_FROM_BUFFER(response_topic_buffer),
        }
      };

      LOG_AND_EXIT_IF_FAILED(az_mqtt_rpc_server_register(&rpc_server, &rpc_server_context, &rpc_server_options));
      break;
    }

    case AZ_MQTT_EVENT_DISCONNECT_RSP:
    {
      connected = false;
      printf(LOG_APP "[%p] DISCONNECTED\n", client);
      break;
    }

    case AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND:
    {
      az_mqtt_rpc_server_pending_command* command_data = (az_mqtt_rpc_server_pending_command*)event.data;
      // function to actually handle command execution
      az_iot_status rc = execute_command(command_data);
      az_mqtt_rpc_server_execution_data return_data = {
        .correlation_id = command_data->correlation_id,
        .response = AZ_SPAN_FROM_STR("dummy payload"),
        .response_topic = command_data->response_topic,
        .status = rc
      };
      LOG_AND_EXIT_IF_FAILED(az_mqtt_rpc_server_execution_finish(&rpc_server, &rpc_server_context, &return_data));
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

  connection_context = az_context_create_with_expiration(
      &az_context_application, az_context_get_expiration(&az_context_application));

  az_mqtt mqtt;
  az_mqtt_options mqtt_options = az_mqtt_options_default();

  LOG_AND_EXIT_IF_FAILED(az_mqtt_init(&mqtt, &mqtt_options));

  az_credential_x509 primary_credential = (az_credential_x509){
    .cert = cert_path1,
    .key = key_path1,
    .key_type = AZ_CREDENTIALS_X509_KEY_MEMORY,
  };

  az_iot_connection_options connection_options = az_iot_connection_options_default();
  connection_options.connection_management = true;
  connection_options.client_id_buffer = AZ_SPAN_FROM_BUFFER(client_id_buffer);
  connection_options.username_buffer = AZ_SPAN_FROM_BUFFER(username_buffer);
  connection_options.password_buffer = AZ_SPAN_EMPTY;
  connection_options.primary_credential = &primary_credential;

  LOG_AND_EXIT_IF_FAILED(az_iot_connection_init(
      &iot_connection, &connection_context, &mqtt, iot_callback, &connection_options));

  LOG_AND_EXIT_IF_FAILED(az_rpc_server_init(&rpc_server, &iot_connection));
  

  LOG_AND_EXIT_IF_FAILED(az_iot_connection_open(&iot_connection));

  for (int i = 45; i > 0; i--)
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

  printf(LOG_APP "Done.                                \n");
  return 0;
}
