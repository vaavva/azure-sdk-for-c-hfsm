/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Mosquitto rpc server that subscribes to 2 commands sample
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

static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to key file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:rpc:samples:vehicle;1");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");

// unlock command rpc server information
static const az_span unlock_command_name = AZ_SPAN_LITERAL_FROM_STR("unlock");
static char sub_topic_buffer[256];
static char unlock_response_payload_buffer[256];
// pending command
static char unlock_correlation_id_buffer[37];
static char unlock_response_topic_buffer[256];
static char unlock_request_topic_buffer[256];
static char unlock_request_payload_buffer[256];
static char unlock_content_type_buffer[256];


static az_mqtt5_rpc_server rpc_server;

static az_mqtt5_rpc_server_execution_req_event_data unlock_pending_command;
#ifdef _WIN32
static timer_t unlock_timer; // placeholder
#else
static timer_t unlock_timer;
static struct sigevent unlock_sev;
static struct itimerspec unlock_trigger;
#endif

// getStats command rpc server information
static const az_span getstats_command_name = AZ_SPAN_LITERAL_FROM_STR("getStats");
// static char getstats_response_payload_buffer[256];
// pending command
static char getstats_correlation_id_buffer[37];
static char getstats_response_topic_buffer[256];
static char getstats_request_topic_buffer[256];
static char getstats_request_payload_buffer[256];
static char getstats_content_type_buffer[256];


static az_mqtt5_rpc_server_execution_req_event_data getstats_pending_command;
#ifdef _WIN32
static timer_t getstats_timer; // placeholder
#else
static timer_t getstats_timer;
static struct sigevent getstats_sev;
static struct itimerspec getstats_trigger;
#endif

enum command_type
{
  RPC_SERVER_UNLOCK,
  RPC_SERVER_GETSTATS
};

// connection details
static az_mqtt5_connection mqtt_connection;
static az_context connection_context;

volatile bool sample_finished = false;

az_mqtt5_rpc_status execute_unlock_command(unlock_request req);
az_mqtt5_rpc_status execute_getstats_command(az_mqtt5_rpc_server_execution_req_event_data command_data);
az_result check_for_commands();
az_result copy_execution_event_data(
    az_mqtt5_rpc_server_execution_req_event_data* destination,
    az_mqtt5_rpc_server_execution_req_event_data source,
    enum command_type command_name);
az_result iot_callback(az_mqtt5_connection* client, az_event event);

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
static void timer_callback(union sigval sv)
{
  #ifdef _WIN32
  return; // AZ_ERROR_DEPENDENCY_NOT_PROVIDED
#else
  void* callback_context = sv.sival_ptr;
#endif
  enum command_type command_name = (enum command_type)callback_context;

  timer_t* timer;
  az_mqtt5_rpc_server_execution_req_event_data* pending_command;
  switch(command_name)
  {
    case RPC_SERVER_UNLOCK:
      printf(LOG_APP_ERROR "unlock execution timed out.\n");
      pending_command = &unlock_pending_command;
      timer = &unlock_timer;
      pending_command->content_type = AZ_SPAN_FROM_BUFFER(unlock_content_type_buffer);
      break;

    case RPC_SERVER_GETSTATS:
      printf(LOG_APP_ERROR "get stats execution timed out.\n");
      pending_command = &getstats_pending_command;
      timer = &getstats_timer;
      pending_command->content_type = AZ_SPAN_FROM_BUFFER(getstats_content_type_buffer);
      break;

    default:
      printf(LOG_APP_ERROR "Unknown command type.\n");
      return;
      break;
  }

  az_mqtt5_rpc_server_execution_rsp_event_data return_data = (az_mqtt5_rpc_server_execution_rsp_event_data){
        .correlation_id = pending_command->correlation_id,
        .error_message = AZ_SPAN_FROM_STR("Command Server timeout"),
        .response_topic = pending_command->response_topic,
        .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
        .request_topic = pending_command->request_topic,
        .response = AZ_SPAN_EMPTY
      };

  if (az_result_failed(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data)))
  {
    printf(LOG_APP_ERROR "Failed sending execution response to HFSM\n");
    return;
  }

  pending_command->correlation_id = AZ_SPAN_EMPTY;

#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  if (0 != timer_delete(*timer))
  {
    printf(LOG_APP_ERROR "Failed destroying timer\n");
    return;
  }
#endif
}

/**
 * @brief Start a timer
 */
AZ_INLINE az_result start_timer(enum command_type command_name, int32_t delay_milliseconds)
{
#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  timer_t* timer;
  struct sigevent* sev;
  struct itimerspec* trigger;
  switch(command_name)
  {
    case RPC_SERVER_UNLOCK:
      timer = &unlock_timer;
      sev = &unlock_sev;
      trigger = &unlock_trigger;
      break;

    case RPC_SERVER_GETSTATS:
      timer = &getstats_timer;
      sev = &getstats_sev;
      trigger = &getstats_trigger;
      break;

    default:
      printf(LOG_APP_ERROR "Unknown command type.\n");
      return AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }

  sev->sigev_notify = SIGEV_THREAD;
  sev->sigev_notify_function = &timer_callback;
  sev->sigev_value.sival_ptr = (void *)command_name;

  if (0 != timer_create(CLOCK_REALTIME, sev, timer))
  {
    // if (ENOMEM == errno)
    // {
    //   return AZ_ERROR_OUT_OF_MEMORY;
    // }
    // else
    // {
    return AZ_ERROR_ARG;
    // }
  }

  // start timer
  trigger->it_value.tv_sec = delay_milliseconds / 1000;
  trigger->it_value.tv_nsec = (delay_milliseconds % 1000) * 1000000;

  if (0 != timer_settime(*timer, 0, trigger, NULL))
  {
    return AZ_ERROR_ARG;
  }
#endif

  return AZ_OK;
}

/**
 * @brief Stop the timer
*/
AZ_INLINE az_result stop_timer(enum command_type command_name)
{
  #ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else

  az_result ret = AZ_OK;
  switch(command_name)
  {
    case RPC_SERVER_UNLOCK:
      if (0 != timer_delete(unlock_timer))
      {
        return AZ_ERROR_ARG;
      }
      break;

    case RPC_SERVER_GETSTATS:
      if (0 != timer_delete(getstats_timer))
      {
        return AZ_ERROR_ARG;
      }
      break;

    default:
      printf(LOG_APP_ERROR "Unknown command type.\n");
      ret = AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }

  return ret;
#endif
}

/**
 * @brief Function that does the actual command execution
 * @note Needs to be modified for your solution
*/
az_mqtt5_rpc_status execute_unlock_command(unlock_request req)
{
  // for now, just print details from the command
  printf(
      LOG_APP "Executing command from: %s at: %ld\n",
      az_span_ptr(req.requested_from),
      req.request_timestamp);
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
az_mqtt5_rpc_status execute_getstats_command(az_mqtt5_rpc_server_execution_req_event_data command_data)
{
  // for now, just print details from the command
  printf(LOG_APP "Executing get stats command to return to: %s\n", az_span_ptr(command_data.response_topic));
  for (int i = 3; i > 0; i--)
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
  if (az_span_ptr(unlock_pending_command.correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    uint8_t copy_buffer[az_span_size(unlock_pending_command.correlation_id)];
    az_span correlation_id_copy = az_span_create(copy_buffer, az_span_size(unlock_pending_command.correlation_id));
    az_span_copy(correlation_id_copy, unlock_pending_command.correlation_id);

    if (!az_span_is_content_equal(content_type, unlock_pending_command.content_type))
    {
      // TODO: should this completely fail execution? This currently matches the C# implementation.
      // I feel like it should send an error response
      printf(
          LOG_APP_ERROR "Invalid content type. Expected: {%s} Actual: {%s}\n",
          az_span_ptr(content_type),
          az_span_ptr(unlock_pending_command.content_type));
      return AZ_ERROR_NOT_SUPPORTED;
    }

    unlock_request req;
    az_mqtt5_rpc_status rc;
    az_span error_message = AZ_SPAN_EMPTY;
    // could check here for the expected request topic to determine which command to execute
    if (az_result_failed(deserialize_unlock_request(unlock_pending_command.request_data, &req)))
    {
      printf(LOG_APP_ERROR "Failed to deserialize request\n");
      rc = AZ_MQTT5_RPC_STATUS_UNSUPPORTED_TYPE;
      error_message = az_span_create_from_str("Command not supported");
    }
    else
    {
      rc = execute_unlock_command(req);
    }

    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, unlock_pending_command.correlation_id))
    {
      stop_timer(RPC_SERVER_UNLOCK);
      az_span response_payload = AZ_SPAN_EMPTY;
      if (rc == AZ_MQTT5_RPC_STATUS_OK)
      {
        // Serialize response
        response_payload = AZ_SPAN_FROM_BUFFER(unlock_response_payload_buffer);
        LOG_AND_EXIT_IF_FAILED(serialize_response_payload(req, response_payload));
      }
      /* Modify the response/error message/status as needed for your solution */
      az_mqtt5_rpc_server_execution_rsp_event_data return_data = {
        .correlation_id = unlock_pending_command.correlation_id,
        .response = response_payload,
        .response_topic = unlock_pending_command.response_topic,
        .status = rc,
        .content_type = content_type,
        .request_topic = unlock_pending_command.request_topic,
        .error_message = error_message
      };
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data));

      unlock_pending_command.content_type = AZ_SPAN_FROM_BUFFER(unlock_content_type_buffer);
      unlock_pending_command.correlation_id = AZ_SPAN_EMPTY;
    }
  }
  if (az_span_ptr(getstats_pending_command.correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    uint8_t copy_buffer[az_span_size(getstats_pending_command.correlation_id)];
    az_span correlation_id_copy = az_span_create(copy_buffer, az_span_size(getstats_pending_command.correlation_id));
    az_span_copy(correlation_id_copy, getstats_pending_command.correlation_id);

    az_mqtt5_rpc_status rc = execute_getstats_command(getstats_pending_command);

    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, getstats_pending_command.correlation_id))
    {
      stop_timer(RPC_SERVER_GETSTATS);
      /* Modify the response/error message/status as needed for your solution */
      az_mqtt5_rpc_server_execution_rsp_event_data return_data = {
        .correlation_id = getstats_pending_command.correlation_id,
        .response = AZ_SPAN_FROM_STR("{\"Items\":[{\"When\":\"2018-01-01T00:00:00Z\",\"Value\":\"yes\"},{\"When\":\"2018-02-01T00:00:00Z\",\"Value\":\"no\"}]}"),
        .response_topic = getstats_pending_command.response_topic,
        .request_topic = getstats_pending_command.request_topic,
        .status = rc,
        .content_type = content_type,
        .error_message = AZ_SPAN_EMPTY
      };
      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data));

      getstats_pending_command.content_type = AZ_SPAN_FROM_BUFFER(getstats_content_type_buffer);
      getstats_pending_command.correlation_id = AZ_SPAN_EMPTY;
    }
  }
  return AZ_OK;
}

az_result copy_execution_event_data(
    az_mqtt5_rpc_server_execution_req_event_data* destination,
    az_mqtt5_rpc_server_execution_req_event_data source,
    enum command_type command_name)
{
  az_span_copy(destination->request_topic, source.request_topic);
  az_span_copy(destination->response_topic, source.response_topic);
  az_span_copy(destination->request_data, source.request_data);
  az_span_copy(destination->content_type, source.content_type);
  destination->content_type
      = az_span_slice(destination->content_type, 0, az_span_size(source.content_type));
  switch(command_name)
  {
    case RPC_SERVER_UNLOCK:
      destination->correlation_id = AZ_SPAN_FROM_BUFFER(unlock_correlation_id_buffer);
      break;

    case RPC_SERVER_GETSTATS:
      destination->correlation_id = AZ_SPAN_FROM_BUFFER(getstats_correlation_id_buffer);
      break;

    default:
      printf(LOG_APP_ERROR "Unknown command type.\n");
      return AZ_ERROR_NOT_IMPLEMENTED;
      break;
  }
  az_span_copy(destination->correlation_id, source.correlation_id);
  destination->correlation_id
      = az_span_slice(destination->correlation_id, 0, az_span_size(source.correlation_id));

  return AZ_OK;
}

/**
 * @brief Callback function for all clients
 * @note If you add other clients, you can add handling for their events here
 */
az_result iot_callback(az_mqtt5_connection* client, az_event event)
{
  (void)client;
  az_app_log_callback(event.type, AZ_SPAN_FROM_STR("APP/callback"));
  switch (event.type)
  {
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    {
      az_mqtt5_connack_data* connack_data = (az_mqtt5_connack_data*)event.data;
      printf(LOG_APP "CONNACK: %d\n", connack_data->connack_reason);

      LOG_AND_EXIT_IF_FAILED(az_mqtt5_rpc_server_register(&rpc_server));
      break;
    }

    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
    {
      printf(LOG_APP "DISCONNECTED\n");
      sample_finished = true;
      break;
    }

    case AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ:
    {
      az_mqtt5_rpc_server_execution_req_event_data* command_data = (az_mqtt5_rpc_server_execution_req_event_data*)event.data;
      az_mqtt5_rpc_server_execution_req_event_data* pending_command;
      enum command_type command_name;

      if (az_span_is_content_equal(unlock_command_name, az_span_slice_to_end(command_data->request_topic, az_span_size(command_data->request_topic) - az_span_size(unlock_command_name))))
      {
        pending_command = &unlock_pending_command;
        command_name = RPC_SERVER_UNLOCK;
      }
      else if (az_span_is_content_equal(getstats_command_name, az_span_slice_to_end(command_data->request_topic, az_span_size(command_data->request_topic) - az_span_size(getstats_command_name))))
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
        printf(LOG_APP "Received %s command while another command is pending. Sending error response.\n", az_span_ptr(command_data->request_topic));
        az_mqtt5_rpc_server_execution_rsp_event_data return_data
            = { .correlation_id = command_data->correlation_id,
                .error_message = AZ_SPAN_FROM_STR("Can't execute more than one command at a time"),
                .response_topic = command_data->response_topic,
                .request_topic = command_data->request_topic,
                .status = AZ_MQTT5_RPC_STATUS_THROTTLED,
                .response = AZ_SPAN_EMPTY,
                .content_type = AZ_SPAN_EMPTY };
        if (az_result_failed(az_mqtt5_rpc_server_execution_finish(&rpc_server, &return_data)))
        {
          printf(LOG_APP_ERROR "Failed sending execution response to HFSM\n");
        }
      }
      else
      {
        // Mark that there's a pending command to be executed
        LOG_AND_EXIT_IF_FAILED(copy_execution_event_data(pending_command, *command_data, command_name));
        start_timer(command_name, 2000);
        printf(LOG_APP "Added %s command to queue\n", az_span_ptr(command_data->request_topic));
      }

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
  connection_options.password_buffer = AZ_SPAN_EMPTY;
  connection_options.hostname = hostname;
  connection_options.client_certificates[0] = primary_credential;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_init(
      &mqtt_connection, &connection_context, &mqtt5, iot_callback, &connection_options));

  unlock_pending_command.request_data = AZ_SPAN_FROM_BUFFER(unlock_request_payload_buffer);
  unlock_pending_command.content_type = AZ_SPAN_FROM_BUFFER(unlock_content_type_buffer);
  unlock_pending_command.correlation_id = AZ_SPAN_EMPTY;
  unlock_pending_command.response_topic = AZ_SPAN_FROM_BUFFER(unlock_response_topic_buffer);
  unlock_pending_command.request_topic = AZ_SPAN_FROM_BUFFER(unlock_request_topic_buffer);

  az_mqtt5_property_bag property_bag;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&property_bag, &mqtt5, NULL));

  az_mqtt5_rpc_server_memory rpc_server_memory
      = (az_mqtt5_rpc_server_memory){ .property_bag = property_bag,
                                      .sub_topic = AZ_SPAN_FROM_BUFFER(sub_topic_buffer) };

  LOG_AND_EXIT_IF_FAILED(az_rpc_server_init(&rpc_server, &mqtt_connection, &rpc_server_memory, model_id, client_id, AZ_SPAN_EMPTY, NULL));

  getstats_pending_command.request_data = AZ_SPAN_FROM_BUFFER(getstats_request_payload_buffer);
  getstats_pending_command.content_type = AZ_SPAN_FROM_BUFFER(getstats_content_type_buffer);
  getstats_pending_command.correlation_id = AZ_SPAN_EMPTY;
  getstats_pending_command.response_topic = AZ_SPAN_FROM_BUFFER(getstats_response_topic_buffer);
  getstats_pending_command.request_topic = AZ_SPAN_FROM_BUFFER(getstats_request_topic_buffer);

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_connection_open(&mqtt_connection));

  // infinite execution loop
  for (int i = 45; !sample_finished && i > 0; i++)
  {
    LOG_AND_EXIT_IF_FAILED(check_for_commands());
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

  if (mqtt5._internal.mosquitto_handle != NULL)
  {
    mosquitto_loop_stop(mqtt5._internal.mosquitto_handle, false);
    mosquitto_destroy(mqtt5._internal.mosquitto_handle);
  }

  // mosquitto allocates the property bag for us, but we're responsible for free'ing it
  mosquitto_property_free_all(&property_bag.properties);

  if (mosquitto_lib_cleanup() != MOSQ_ERR_SUCCESS)
  {
    printf(LOG_APP "Failed to cleanup MosquittoLib\n");
    return -1;
  }

  printf(LOG_APP "Done.                                \n");
  return 0;
}