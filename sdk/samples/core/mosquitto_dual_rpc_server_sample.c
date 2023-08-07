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
#include <azure/core/az_log.h>

static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to pem file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span password = AZ_SPAN_EMPTY;
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:rpc:samples:vehicle;1");

// unlock command rpc server information
static const az_span unlock_command_name = AZ_SPAN_LITERAL_FROM_STR("unlock");
static char unlock_correlation_id_buffer[37];
static char unlock_response_topic_buffer[256];
static char unlock_sub_topic_buffer[256];

static az_mqtt5_rpc_server unlock_rpc_server;
static az_mqtt5_rpc_server_options unlock_rpc_server_options;

static az_mqtt5_rpc_server_command_data unlock_pending_command;
static _az_platform_timer unlock_timer;

// getStats command rpc server information
static const az_span getstats_command_name = AZ_SPAN_LITERAL_FROM_STR("getStats");
static char getstats_correlation_id_buffer[37];
static char getstats_response_topic_buffer[256];
static char getstats_sub_topic_buffer[256];

static az_mqtt5_rpc_server getstats_rpc_server;
static az_mqtt5_rpc_server_options getstats_rpc_server_options;

static az_mqtt5_rpc_server_command_data getstats_pending_command;
static _az_platform_timer getstats_timer;

enum command_type
{
  RPC_SERVER_UNLOCK,
  RPC_SERVER_GETSTATS
};

// connection details
static az_mqtt5_connection iot_connection;
static az_context connection_context;

volatile bool connected = false;

void az_platform_critical_error()
{
  printf(LOG_APP "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

/**
 * @brief On command timeout, send an error response with timeout details to the HFSM
 * @note May need to be modified for your solution
*/
static void timer_callback(void* callback_context)
{
  enum command_type command_name = (enum command_type)callback_context;
  
  az_mqtt5_rpc_server execution_server;
  _az_platform_timer* timer;
  az_mqtt5_rpc_server_command_data* pending_command;
  switch(command_name)
  {
    case RPC_SERVER_UNLOCK:
      printf(LOG_APP_ERROR "unlock execution timed out.\n");
      pending_command = &unlock_pending_command;
      execution_server = unlock_rpc_server;
      timer = &unlock_timer;
      break;

    case RPC_SERVER_GETSTATS:
      printf(LOG_APP_ERROR "get stats execution timed out.\n");
      pending_command = &getstats_pending_command;
      execution_server = getstats_rpc_server;
      timer = &getstats_timer;
      break;

    default:
      printf(LOG_APP_ERROR "Unknown command type.\n");
      return;
      break;
  }

  az_mqtt5_rpc_server_execution_data return_data = (az_mqtt5_rpc_server_execution_data){
        .correlation_id = pending_command->correlation_id,
        .error_message = AZ_SPAN_FROM_STR("Command Server timeout"),
        .response_topic = pending_command->response_topic,
        .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
        .response = AZ_SPAN_EMPTY
      };

  if (az_result_failed(az_mqtt5_rpc_server_execution_finish(&execution_server, &return_data)))
  {
    printf(LOG_APP_ERROR "Failed sending execution response to HFSM\n");
    return;
  }

  pending_command->correlation_id = AZ_SPAN_EMPTY;

  if (az_result_failed(az_platform_timer_destroy(timer)))
  {
    printf(LOG_APP_ERROR "Failed destroying timer\n");
    return;
  }
}

/**
 * @brief Start a timer
 */
AZ_INLINE az_result start_timer(enum command_type command_name, int32_t delay_milliseconds)
{
  _az_platform_timer* timer;
  switch(command_name)
  {
    case RPC_SERVER_UNLOCK:
      timer = &unlock_timer;
      break;

    case RPC_SERVER_GETSTATS:
      timer = &getstats_timer;
      break;

    default:
      printf(LOG_APP_ERROR "Unknown command type.\n");
      return AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }

  LOG_AND_EXIT_IF_FAILED(az_platform_timer_create(timer, timer_callback, (void *)command_name));

  LOG_AND_EXIT_IF_FAILED(az_platform_timer_start(timer, delay_milliseconds));
  return AZ_OK;
}

/**
 * @brief Stop the timer
*/
AZ_INLINE az_result stop_timer(enum command_type command_name)
{
  az_result ret = AZ_OK;
  switch(command_name)
  {
    case RPC_SERVER_UNLOCK:
      ret = az_platform_timer_destroy(&unlock_timer);
      break;

    case RPC_SERVER_GETSTATS:
      ret = az_platform_timer_destroy(&getstats_timer);
      break;

    default:
      printf(LOG_APP_ERROR "Unknown command type.\n");
      ret = AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }

  return ret;
}

/**
 * @brief Function that does the actual command execution
 * @note Needs to be modified for your solution
*/
az_mqtt5_rpc_status execute_unlock_command(az_mqtt5_rpc_server_command_data command_data)
{
  // for now, just print details from the command
  printf(LOG_APP "Executing unlock command to return to: %s\n", az_span_ptr(command_data.response_topic));
  for (int i = 5; i > 0; i--)
  {
    LOG_AND_EXIT_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Executing unlock %ds        \r", i);
    fflush(stdout);
  }
  printf(LOG_APP "Finished unlock execution\n");
  return AZ_MQTT5_RPC_STATUS_OK;
}

/**
 * @brief Function that does the actual command execution
 * @note Needs to be modified for your solution
*/
az_mqtt5_rpc_status execute_getstats_command(az_mqtt5_rpc_server_command_data command_data)
{
  // for now, just print details from the command
  printf(LOG_APP "Executing get stats command to return to: %s\n", az_span_ptr(command_data.response_topic));
  for (int i = 1; i > 0; i--)
  {
    LOG_AND_EXIT_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Executing get stats %ds        \r", i);
    fflush(stdout);
  }
  printf(LOG_APP "Finished getstats execution\n");
  return AZ_MQTT5_RPC_STATUS_OK;
}

/**
 * @brief Check if there is a pending command and execute it. On completion, if the command hasn't
 * timed out, send the result back to the hfsm
 * @note Result to be sent back to the hfsm needs to be modified for your solution
 */
az_result check_for_commands()
{
  if (az_span_ptr(getstats_pending_command.correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    char copy_buffer[az_span_size(getstats_pending_command.correlation_id)];
    az_span correlation_id_copy = az_span_create(copy_buffer, az_span_size(getstats_pending_command.correlation_id));
    az_span_copy(correlation_id_copy, getstats_pending_command.correlation_id);

    az_mqtt5_rpc_status rc = execute_getstats_command(getstats_pending_command);
    
    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, getstats_pending_command.correlation_id))
    {
      stop_timer(RPC_SERVER_GETSTATS);
      /* Modify the response/error message/status as needed for your solution */
      az_mqtt5_rpc_server_execution_data return_data = {
        .correlation_id = getstats_pending_command.correlation_id,
        .response = AZ_SPAN_FROM_STR("{\"Items\":[{\"When\":\"2018-01-01T00:00:00Z\",\"Value\":\"yes\"},{\"When\":\"2018-02-01T00:00:00Z\",\"Value\":\"no\"}]}"),
        .response_topic = getstats_pending_command.response_topic,
        .status = rc,
        .error_message = AZ_SPAN_EMPTY
      };
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_execution_finish(&getstats_rpc_server, &return_data));

      getstats_pending_command.correlation_id = AZ_SPAN_EMPTY;
    }
  }
  if (az_span_ptr(unlock_pending_command.correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    char copy_buffer[az_span_size(unlock_pending_command.correlation_id)];
    az_span correlation_id_copy = az_span_create(copy_buffer, az_span_size(unlock_pending_command.correlation_id));
    az_span_copy(correlation_id_copy, unlock_pending_command.correlation_id);

    az_mqtt5_rpc_status rc = execute_unlock_command(unlock_pending_command);
    
    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, unlock_pending_command.correlation_id))
    {
      stop_timer(RPC_SERVER_UNLOCK);
      /* Modify the response/error message/status as needed for your solution */
      az_mqtt5_rpc_server_execution_data return_data = {
        .correlation_id = unlock_pending_command.correlation_id,
        .response = AZ_SPAN_FROM_STR("{\"Succeed\":true,\"ReceivedFrom\":\"mobile-app\",\"ProcessedMs\":5.0}"),
        .response_topic = unlock_pending_command.response_topic,
        .status = rc,
        .error_message = AZ_SPAN_EMPTY
      };
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_execution_finish(&unlock_rpc_server, &return_data));

      unlock_pending_command.correlation_id = AZ_SPAN_EMPTY;
    }
  }
  return AZ_OK;
}

/**
 * @brief Callback function for all clients
 * @note If you add other clients, you can add handling for their events here
 */
az_result iot_callback(az_mqtt5_connection* client, az_event event)
{
  az_app_log_callback(event.type, AZ_SPAN_FROM_STR("APP/callback"));
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      connected = true;
      az_mqtt5_connack_data* connack_data = (az_mqtt5_connack_data*)event.data;
      printf(LOG_APP "[%p] CONNACK: %d\n", client, connack_data->connack_reason);

      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_register(&unlock_rpc_server));
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_register(&getstats_rpc_server));
      break;
    }

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    {
      connected = false;
      printf(LOG_APP "[%p] DISCONNECTED\n", client);
      break;
    }

    case AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND:
    {
      az_mqtt5_rpc_server_command_data* command_data = (az_mqtt5_rpc_server_command_data*)event.data;
      az_mqtt5_rpc_server_command_data* pending_command;
      enum command_type command_name;

      if (az_span_is_content_equal(command_data->command_name, unlock_command_name))
      {
        pending_command = &unlock_pending_command;
        command_name = RPC_SERVER_UNLOCK;
      }
      else if (az_span_is_content_equal(command_data->command_name, getstats_command_name))
      {
        pending_command = &getstats_pending_command;
        command_name = RPC_SERVER_GETSTATS;
      }
      else
      {
        printf(LOG_APP "Received unknown command. Ignoring.\n");
        break;
      }

      if (az_span_ptr(pending_command->correlation_id) != NULL)
      {
        printf(LOG_APP "Received %s command while another command is pending. Ignoring.\n", az_span_ptr(command_data->command_name));
      }
      else
      {
        // Mark that there's a pending command to be executed
        *pending_command = *(az_mqtt5_rpc_server_command_data*)event.data;
        start_timer(command_name, 10000);
        printf(LOG_APP "Added %s command to queue\n", az_span_ptr(command_data->command_name));
      }

      break;
    }

    case AZ_EVENT_RPC_SERVER_UNHANDLED_COMMAND:
    {
      az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;
      printf(LOG_APP "Received unhandled command.\n");
      // could put this command in a queue and trigger a AZ_MQTT5_EVENT_PUB_RECV_IND with it once
      // we're finished with the current command
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

  az_mqtt5 mqtt5;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_init(&mqtt5, NULL));

  az_mqtt5_x509_client_certificate primary_credential = (az_mqtt5_x509_client_certificate){
    .cert = cert_path1,
    .key = key_path1,
  };

  az_mqtt5_connection_options connection_options = az_mqtt5_connection_options_default();
  connection_options.client_id_buffer = client_id;
  connection_options.username_buffer = username;
  connection_options.password_buffer = password;
  connection_options.hostname = hostname;
  connection_options.client_certificates[0] = primary_credential;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &iot_connection, &connection_context, &mqtt5, iot_callback, &connection_options));

  az_mqtt5_property_bag unlock_property_bag;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&unlock_property_bag, &mqtt5, NULL));

  unlock_rpc_server_options = (az_mqtt5_rpc_server_options){
    .sub_topic = AZ_SPAN_FROM_BUFFER(unlock_sub_topic_buffer),
    .command_name = unlock_command_name,
    .model_id = model_id
  };

  az_mqtt5_rpc_server_data unlock_rpc_server_data = (az_mqtt5_rpc_server_data){
    .property_bag = unlock_property_bag
  };

  LOG_AND_EXIT_IF_FAILED(az_rpc_server_init(&unlock_rpc_server, &iot_connection, &unlock_rpc_server_options, &unlock_rpc_server_data));

  az_mqtt5_property_bag getstats_property_bag;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&getstats_property_bag, &mqtt5, NULL));
  
  getstats_rpc_server_options = (az_mqtt5_rpc_server_options){
    .sub_topic = AZ_SPAN_FROM_BUFFER(getstats_sub_topic_buffer),
    .command_name = getstats_command_name,
    .model_id = model_id
  };

  az_mqtt5_rpc_server_data getstats_rpc_server_data = (az_mqtt5_rpc_server_data){
    .property_bag = getstats_property_bag
  };

  LOG_AND_EXIT_IF_FAILED(az_rpc_server_init(&getstats_rpc_server, &iot_connection, &getstats_rpc_server_options, &getstats_rpc_server_data));


  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&iot_connection));

  // infinite execution loop
  for (int i = 45; i > 0; i++)
  {
    LOG_AND_EXIT_IF_FAILED(check_for_commands());
    LOG_AND_EXIT_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Waiting...\r");
    fflush(stdout);
  }

  // clean-up functions shown for completeness
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_close(&iot_connection));

  for (int i = 15; connected && i > 0; i--)
  {
    LOG_AND_EXIT_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Waiting for disconnect %ds        \r", i);
    fflush(stdout);
  }

  mosquitto_property_free_all(&unlock_property_bag.properties);
  mosquitto_property_free_all(&getstats_property_bag.properties);

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                                \n");
  return 0;
}
