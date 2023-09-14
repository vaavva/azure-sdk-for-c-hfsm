/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Mosquitto rpc server sample
 *
 */

#include <az_log_listener.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "mosquitto_rpc_server_sample_json_parser.h"
#include <azure/az_core.h>
#include <azure/core/az_log.h>

#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

// User-defined parameters
static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to cert key file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span command_name = AZ_SPAN_LITERAL_FROM_STR("unlock");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:rpc:samples:vehicle;1");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");

// Static memory allocation.
static char subscription_topic_buffer[256];
static char response_payload_buffer[256];

// for pending_command
static char correlation_id_buffer[256];
static char response_topic_buffer[256];
static char request_topic_buffer[256];
static char request_payload_buffer[256];
static char content_type_buffer[256];

// State variables
static az_mqtt5_connection mqtt_connection;
static az_context connection_context;

static az_mqtt5_rpc_server rpc_server;
static az_mqtt5_rpc_server_policy rpc_server_policy;

volatile bool sample_finished = false;

static az_mqtt5_rpc_server_pending_command pending_server_command;

az_mqtt5_rpc_status execute_command(az_span request_data, az_span request_topic, az_span response);

az_result mqtt_callback(az_mqtt5_connection* client, az_event event);

void az_platform_critical_error()
{
  printf(LOG_APP_ERROR "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

/**
 * @brief Function that does the actual command execution, including deserializing the request and serializing the response
 * @note Needs to be modified for your solution
 */
az_mqtt5_rpc_status execute_command(az_span request_data, az_span request_topic, az_span response)
{
  (void)request_topic; // used to determine what command to execute
  unlock_request req;
  az_mqtt5_rpc_status rc = AZ_MQTT5_RPC_STATUS_OK;
  // deserialize
  if (az_result_failed(deserialize_unlock_request(request_data, &req)))
  {
    printf(LOG_APP_ERROR "Failed to deserialize request\n");
    az_span_copy(response, az_span_create_from_str("Failed to deserialize unlock command request."));
    return AZ_MQTT5_RPC_STATUS_UNSUPPORTED_TYPE;
  }

  // execute
  // for now, just print details from the command
  printf(
      LOG_APP "Executing command from: %s at: %ld\n",
      az_span_ptr(req.requested_from),
      req.request_timestamp);

  // serialize response
  serialize_response_payload(req, response);
  return rc;
}

/**
 * @brief MQTT client callback function for all clients
 * @note If you add other clients, you can add handling for their events here
 */
az_result mqtt_callback(az_mqtt5_connection* client, az_event event)
{
  (void)client;
  az_app_log_callback(event.type, AZ_SPAN_FROM_STR("APP/callback"));
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      az_mqtt5_connack_data* connack_data = (az_mqtt5_connack_data*)event.data;
      printf(LOG_APP "CONNACK: %d\n", connack_data->connack_reason);

      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_register(&rpc_server_policy));
      break;
    }

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    {
      printf(LOG_APP "DISCONNECTED\n");
      sample_finished = true;
      break;
    }

    case AZ_MQTT5_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ:
    {
      az_mqtt5_rpc_server_execution_req_event_data data
          = *(az_mqtt5_rpc_server_execution_req_event_data*)event.data;
      // can check here for the expected request topic to determine which command to execute
        // Mark that there's a pending command to be executed
        LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_pending_command_add(&pending_server_command, data, 2000, &rpc_server_policy));
        printf(LOG_APP "Added command to queue\n");
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

  printf(LOG_APP "Using MosquittoLib version %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  connection_context = az_context_create_with_expiration(
      &az_context_application, az_context_get_expiration(&az_context_application));

  az_mqtt5 mqtt5;
  struct mosquitto* mosq = NULL;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_init(&mqtt5, &mosq, NULL));

  az_mqtt5_x509_client_certificate primary_credential = (az_mqtt5_x509_client_certificate){
    .cert = cert_path1,
    .key = key_path1,
  };

  az_mqtt5_connection_options connection_options = az_mqtt5_connection_options_default();
  connection_options.client_id_buffer = client_id;
  connection_options.username_buffer = username;
  connection_options.password_buffer = AZ_SPAN_EMPTY;
  connection_options.hostname = hostname;
  connection_options.client_certificates[0] = primary_credential;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &mqtt_connection, &connection_context, &mqtt5, mqtt_callback, &connection_options));

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_pending_command_init(&pending_server_command,
    AZ_SPAN_FROM_BUFFER(content_type_buffer),
    AZ_SPAN_FROM_BUFFER(response_topic_buffer),
    AZ_SPAN_FROM_BUFFER(request_topic_buffer),
    AZ_SPAN_FROM_BUFFER(correlation_id_buffer),
    AZ_SPAN_FROM_BUFFER(request_payload_buffer),
    AZ_SPAN_FROM_BUFFER(response_payload_buffer)
    ));

  az_mqtt5_property_bag property_bag;
  mosquitto_property* mosq_prop = NULL;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&property_bag, &mqtt5, &mosq_prop));

  LOG_AND_EXIT_IF_FAILED(az_rpc_server_policy_init(
      &rpc_server_policy,
      &rpc_server,
      &mqtt_connection,
      property_bag,
      AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
      model_id,
      client_id,
      command_name,
      NULL));

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&mqtt_connection));

  // infinite execution loop
  for (int i = 45; !sample_finished && i > 0; i++)
  {
    LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_pending_command_check_and_execute(&pending_server_command,
    content_type,
    execute_command));
#ifdef _WIN32
    Sleep((DWORD)1000);
#else
    sleep(1);
#endif
    printf(LOG_APP "Waiting...\r");
    fflush(stdout);
  }

  // clean-up functions shown for completeness
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_close(&mqtt_connection));

  if (mosq != NULL)
  {
    mosquitto_loop_stop(mosq, false);
    mosquitto_destroy(mosq);
  }

  // mosquitto allocates the property bag for us, but we're responsible for free'ing it
  mosquitto_property_free_all(&mosq_prop);

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                                \n");
  return 0;
}
