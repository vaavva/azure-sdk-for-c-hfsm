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

static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to pem file>");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("<path to key file>");
static const az_span client_id = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span username = AZ_SPAN_LITERAL_FROM_STR("vehicle03");
static const az_span hostname = AZ_SPAN_LITERAL_FROM_STR("<hostname>");
static const az_span command_name = AZ_SPAN_LITERAL_FROM_STR("unlock");
static const az_span model_id = AZ_SPAN_LITERAL_FROM_STR("dtmi:rpc:samples:vehicle;1");
static const az_span content_type = AZ_SPAN_LITERAL_FROM_STR("application/json");

static char subscription_topic_buffer[256];
static char response_payload_buffer[256];

// for pending_command
static char correlation_id_buffer[37];
static char response_topic_buffer[256];
static char command_name_buffer[256];
static char model_id_buffer[256];
static char request_payload_buffer[256];
static char content_type_buffer[256];

static az_mqtt5_rpc_server rpc_server;

volatile bool sample_finished = false;

static az_mqtt5 mqtt5;

static az_mqtt5_rpc_server_command_request pending_command;

#ifdef _WIN32
static timer_t timer; // placeholder
#else
static timer_t timer;
static struct sigevent sev;
static struct itimerspec trigger;
#endif

az_mqtt5_rpc_status execute_command(unlock_request req);
az_result check_for_commands();
az_result copy_execution_event_data(
    az_mqtt5_rpc_server_command_request* destination,
    az_mqtt5_rpc_server_command_request source);

void az_platform_critical_error()
{
  printf(LOG_APP_ERROR "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

/**
 * @brief On command timeout, send an error response with timeout details to the client
 * @note May need to be modified for your solution
 */
static void timer_callback(union sigval sv)
{
#ifdef _WIN32
  return; // AZ_ERROR_DEPENDENCY_NOT_PROVIDED
#else
  // void* callback_context = sv.sival_ptr;
  (void)sv;
#endif

  printf(LOG_APP_ERROR "Command execution timed out.\n");
  az_mqtt5_pub_data pub_data;
  az_mqtt5_rpc_server_response_data return_data
      = { .correlation_id = pending_command.correlation_id,
          .error_message = AZ_SPAN_FROM_STR("Command Server timeout"),
          .response_topic = pending_command.response_topic,
          .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
          .response = AZ_SPAN_EMPTY,
          .content_type = AZ_SPAN_EMPTY };

  if (AZ_OK != az_rpc_server_get_response_packet(&rpc_server, &return_data, &pub_data))
  {
    printf(LOG_APP_ERROR "Failed to create response packet\n");
    return;
  }

  az_result ret = az_mqtt5_outbound_pub(&mqtt5, &pub_data);
  (void)ret;

  az_rpc_server_empty_property_bag(&rpc_server);

  pending_command.content_type = AZ_SPAN_FROM_BUFFER(content_type_buffer);
  pending_command.command_name = AZ_SPAN_FROM_BUFFER(command_name_buffer);
  pending_command.model_id = AZ_SPAN_FROM_BUFFER(model_id_buffer);
  pending_command.correlation_id = AZ_SPAN_EMPTY;

#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  if (0 != timer_delete(timer))
  {
    printf(LOG_APP_ERROR "Failed destroying timer\n");
    return;
  }
#endif
}

/**
 * @brief Start a timer
 */
AZ_INLINE az_result start_timer(void* callback_context, int32_t delay_milliseconds)
{
#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_notify_function = &timer_callback;
  sev.sigev_value.sival_ptr = &callback_context;
  if (0 != timer_create(CLOCK_REALTIME, &sev, &timer))
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
  trigger.it_value.tv_sec = delay_milliseconds / 1000;
  trigger.it_value.tv_nsec = (delay_milliseconds % 1000) * 1000000;

  if (0 != timer_settime(timer, 0, &trigger, NULL))
  {
    return AZ_ERROR_ARG;
  }
#endif

  return AZ_OK;
}

/**
 * @brief Stop the timer
 */
AZ_INLINE az_result stop_timer()
{
#ifdef _WIN32
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
#else
  if (0 != timer_delete(timer))
  {
    return AZ_ERROR_ARG;
  }
#endif

  return AZ_OK;
}

static void on_connect(
    struct mosquitto* mosq,
    void* obj,
    int reason_code,
    int flags,
    const mosquitto_property* props)
{
  (void)obj;
  (void)flags;
  (void)props;

  printf(LOG_APP "CONNACK: %s\n", mosquitto_reason_string(reason_code));
  int rc;
  if (reason_code != 0)
  {
    sample_finished = true;
    /* If the connection fails for any reason, we don't want to keep on
     * retrying in this example, so disconnect. Without this, the client
     * will attempt to reconnect. */
    
    if ((rc = mosquitto_disconnect_v5(mosq, reason_code, NULL)) != MOSQ_ERR_SUCCESS)
    {
      printf(LOG_APP "Failure on disconnect: %s", mosquitto_strerror(rc));
    }
  }

  az_mqtt5_sub_data subscription_data = { .topic_filter = rpc_server.subscription_topic,
                                          .qos = rpc_server.options.subscribe_qos,
                                          .out_id = 0 };

  // send subscribe
  rc = az_mqtt5_outbound_sub(&mqtt5, &subscription_data);
}

static void on_message(
    struct mosquitto* mosq,
    void* obj,
    const struct mosquitto_message* message,
    const mosquitto_property* props)
{
  (void)obj;
  (void)mosq;

  az_result ret;
  az_mqtt5_property_bag property_bag;
  az_mqtt5_property_bag_options property_bag_options;

  property_bag_options = az_mqtt5_property_bag_options_default();
  property_bag_options.properties = (mosquitto_property*)(uintptr_t)props;

  ret = az_mqtt5_property_bag_init(&property_bag, NULL, &property_bag_options);

  if (az_result_failed(ret))
  {
    az_platform_critical_error();
  }

  az_mqtt5_recv_data recv_data = (az_mqtt5_recv_data){ .qos = (int8_t)message->qos,
                             .id = (int32_t)message->mid,
                             .payload = az_span_create(message->payload, message->payloadlen),
                             .topic = az_span_create_from_str(message->topic),
                             .properties = &property_bag };

  az_mqtt5_rpc_server_command_request command_data;
  az_mqtt5_rpc_server_property_pointers props_to_free;
  ret = az_rpc_server_parse_request_topic_and_properties(&rpc_server, &recv_data, &props_to_free, &command_data);

  if (az_span_ptr(pending_command.correlation_id) != NULL)
  {
    // can add this command to a queue to be executed if the application supports executing
    // multiple commands at once.
    printf(LOG_APP
            "Received command while another command is executing. Sending error response.\n");
    az_mqtt5_pub_data pub_data;
    az_mqtt5_rpc_server_response_data response_data
        = { .correlation_id = command_data.correlation_id,
            .error_message = AZ_SPAN_FROM_STR("Can't execute more than one command at a time"),
            .response_topic = command_data.response_topic,
            .status = AZ_MQTT5_RPC_STATUS_THROTTLED,
            .response = AZ_SPAN_EMPTY,
            .content_type = AZ_SPAN_EMPTY };
    ret = az_rpc_server_get_response_packet(&rpc_server, &response_data, &pub_data);

    ret = az_mqtt5_outbound_pub(&mqtt5, &pub_data);

    az_rpc_server_empty_property_bag(&rpc_server);
  }
  else
  {
    // Mark that there's a pending command to be executed
    ret = copy_execution_event_data(&pending_command, command_data);
    start_timer(NULL, 10000);
    printf(LOG_APP "Added command to queue\n");
  }

  if (ret != AZ_OK)
  {
    printf(LOG_APP_ERROR "Failure handling an incoming command\n");
  }

  az_rpc_server_free_properties(props_to_free);
}

/**
 * @brief Function that does the actual command execution
 * @note Needs to be modified for your solution
 */
az_mqtt5_rpc_status execute_command(unlock_request req)
{
  // for now, just print details from the command
  printf(
      LOG_APP "Executing command from: %s at: %ld\n",
      az_span_ptr(req.requested_from),
      req.request_timestamp);
  return AZ_MQTT5_RPC_STATUS_OK;
}

/**
 * @brief Check if there is a pending command and execute it. On completion, if the command hasn't
 * timed out, send the result back to the client
 * @note Result to be sent back needs to be modified for your solution
 */
az_result check_for_commands()
{
  if (az_span_ptr(pending_command.correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    uint8_t copy_buffer[az_span_size(pending_command.correlation_id)];
    az_span correlation_id_copy
        = az_span_create(copy_buffer, az_span_size(pending_command.correlation_id));
    az_span_copy(correlation_id_copy, pending_command.correlation_id);

    if (!az_span_is_content_equal(content_type, pending_command.content_type))
    {
      // TODO: should this completely fail execution? This currently matches the C# implementation.
      // I feel like it should send an error response
      printf(
          LOG_APP_ERROR "Invalid content type. Expected: {%s} Actual: {%s}\n",
          az_span_ptr(content_type),
          az_span_ptr(pending_command.content_type));
      return AZ_ERROR_NOT_SUPPORTED;
    }

    unlock_request req;
    az_mqtt5_rpc_status rc;
    az_span error_message = AZ_SPAN_EMPTY;

    if (az_result_failed(deserialize_unlock_request(pending_command.request_data, &req)))
    {
      printf(LOG_APP_ERROR "Failed to deserialize request\n");
      rc = AZ_MQTT5_RPC_STATUS_UNSUPPORTED_TYPE;
      error_message = az_span_create_from_str("Failed to deserialize unlock command request.");
    }
    else
    {
      rc = execute_command(req);
    }

    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, pending_command.correlation_id))
    {
      stop_timer();
      az_span response_payload = AZ_SPAN_EMPTY;
      if (rc == AZ_MQTT5_RPC_STATUS_OK)
      {
        // Serialize response
        response_payload = AZ_SPAN_FROM_BUFFER(response_payload_buffer);
        LOG_AND_EXIT_IF_FAILED(serialize_response_payload(req, response_payload));
      }

      /* Modify the response/error message/status as needed for your solution */
      az_mqtt5_rpc_server_response_data return_data
          = { .correlation_id = pending_command.correlation_id,
              .response = response_payload,
              .response_topic = pending_command.response_topic,
              .status = rc,
              .content_type = content_type,
              .error_message = error_message };
      
      az_mqtt5_pub_data pub_data;
      rc = az_rpc_server_get_response_packet(&rpc_server, &return_data, &pub_data);

      rc = az_mqtt5_outbound_pub(&mqtt5, &pub_data);

      az_rpc_server_empty_property_bag(&rpc_server);

      pending_command.content_type = AZ_SPAN_FROM_BUFFER(content_type_buffer);
      pending_command.command_name = AZ_SPAN_FROM_BUFFER(command_name_buffer);
      pending_command.model_id = AZ_SPAN_FROM_BUFFER(model_id_buffer);
      pending_command.correlation_id = AZ_SPAN_EMPTY;
    }
  }
  return AZ_OK;
}

az_result copy_execution_event_data(
    az_mqtt5_rpc_server_command_request* destination,
    az_mqtt5_rpc_server_command_request source)
{
  az_span_copy(destination->command_name, source.command_name);
  destination->command_name
      = az_span_slice(destination->command_name, 0, az_span_size(source.command_name));
  az_span_copy(destination->model_id, source.model_id);
  destination->model_id
      = az_span_slice(destination->model_id, 0, az_span_size(source.model_id));
  // az_span_copy(destination->target_client_id, source.target_client_id);
  // destination->target_client_id
  //     = az_span_slice(destination->target_client_id, 0, az_span_size(source.target_client_id));
  az_span_copy(destination->response_topic, source.response_topic);
  az_span_copy(destination->request_data, source.request_data);
  az_span_copy(destination->content_type, source.content_type);
  destination->content_type
      = az_span_slice(destination->content_type, 0, az_span_size(source.content_type));
  destination->correlation_id = AZ_SPAN_FROM_BUFFER(correlation_id_buffer);
  az_span_copy(destination->correlation_id, source.correlation_id);
  destination->correlation_id
      = az_span_slice(destination->correlation_id, 0, az_span_size(source.correlation_id));

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

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_init(&mqtt5, NULL));

  pending_command.request_data = AZ_SPAN_FROM_BUFFER(request_payload_buffer);
  pending_command.content_type = AZ_SPAN_FROM_BUFFER(content_type_buffer);
  pending_command.correlation_id = AZ_SPAN_EMPTY;
  pending_command.response_topic = AZ_SPAN_FROM_BUFFER(response_topic_buffer);
  pending_command.command_name = AZ_SPAN_FROM_BUFFER(command_name_buffer);
  pending_command.model_id = AZ_SPAN_FROM_BUFFER(model_id_buffer);

  az_mqtt5_property_bag property_bag;
  LOG_AND_EXIT_IF_FAILED(az_mqtt5_property_bag_init(&property_bag, NULL, NULL));

  LOG_AND_EXIT_IF_FAILED(az_rpc_server_init(
      &rpc_server,
      property_bag,
      model_id,
      client_id,
      command_name,
      AZ_SPAN_FROM_BUFFER(subscription_topic_buffer),
      NULL));

  az_mqtt5_connect_data connect_data = (az_mqtt5_connect_data){
    .host = hostname,
    .port = AZ_MQTT5_DEFAULT_CONNECT_PORT,
    .client_id = client_id,
    .username = username,
    .password = AZ_SPAN_EMPTY,
    .certificate.cert = cert_path1,
    .certificate.key = key_path1,
    .properties = NULL,
  };

  az_mqtt5_callbacks callbacks = az_mqtt5_callbacks_default();
  callbacks.on_connect = on_connect;
  callbacks.on_message = on_message;
  // These should be implemented in a full solution
  callbacks.on_disconnect = NULL;
  callbacks.on_publish = NULL;
  callbacks.on_subscribe = NULL;

  LOG_AND_EXIT_IF_FAILED(az_mqtt5_outbound_connect(&mqtt5, &connect_data, &callbacks));

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
