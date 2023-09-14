// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_mqtt5_rpc_server.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_mqtt5_rpc_server_pending_command_init(
    az_mqtt5_rpc_server_pending_command* pending_server_command,
    az_span content_type_buffer,
    az_span response_topic_buffer,
    az_span request_topic_buffer,
    az_span correlation_id_buffer,
    az_span request_data_buffer,
    az_span response_payload_buffer
    )
{
  _az_RETURN_IF_FAILED(az_platform_mutex_init(&pending_server_command->mutex));
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pending_server_command->mutex));
  pending_server_command->content_type_buffer = content_type_buffer;
  pending_server_command->response_topic_buffer = response_topic_buffer;
  pending_server_command->request_topic_buffer = request_topic_buffer;
  pending_server_command->correlation_id_buffer = correlation_id_buffer;
  pending_server_command->request_data_buffer = request_data_buffer;
  pending_server_command->response_payload_buffer = response_payload_buffer;
  pending_server_command->rpc_server_policy = NULL;
  pending_server_command->content_type = content_type_buffer;
  pending_server_command->response_topic = response_topic_buffer;
  pending_server_command->request_topic = request_topic_buffer;
  pending_server_command->request_data = request_data_buffer;
  pending_server_command->correlation_id = AZ_SPAN_EMPTY;
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_server_command->mutex));
  return AZ_OK;
}

/**
 * @brief On command timeout, send an error response with timeout details to the HFSM
 * @note May need to be modified for your solution
 */
static void timer_callback(void* callback_context)
{
  az_mqtt5_rpc_server_pending_command* pending_server_command = (az_mqtt5_rpc_server_pending_command*)callback_context;
  if (az_result_failed(az_platform_mutex_acquire(&pending_server_command->mutex)))
  {
    return;
  }
  // printf(LOG_APP_ERROR "Command execution timed out.\n");
  az_mqtt5_rpc_server_execution_rsp_event_data return_data
      = { .correlation_id = pending_server_command->correlation_id,
          .error_message = AZ_SPAN_FROM_STR("Command Server timeout"),
          .response_topic = pending_server_command->response_topic,
          .request_topic = pending_server_command->request_topic,
          .status = AZ_MQTT5_RPC_STATUS_TIMEOUT,
          .response = AZ_SPAN_EMPTY,
          .content_type = AZ_SPAN_EMPTY };
  if (az_result_failed(az_mqtt5_rpc_server_execution_finish(pending_server_command->rpc_server_policy, &return_data)))
  {
    // printf(LOG_APP_ERROR "Failed sending execution response to HFSM\n");
    return;
  }

  pending_server_command->content_type = pending_server_command->content_type_buffer;
  pending_server_command->request_topic = pending_server_command->request_topic_buffer;
  pending_server_command->correlation_id = AZ_SPAN_EMPTY;

  if (az_result_failed(az_platform_timer_destroy(&pending_server_command->timer)))
  {
    printf("Failed to destroy timer\n");
  }
  if (az_result_failed(az_platform_mutex_release(&pending_server_command->mutex)))
  {
    printf("Failed to release mutex\n");
  }
  return;
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_pending_command_add(
    az_mqtt5_rpc_server_pending_command* pending_server_command,
    az_mqtt5_rpc_server_execution_req_event_data data,
    int32_t delay_milliseconds,
    az_mqtt5_rpc_server_policy* rpc_server_policy
    )
{
  az_result ret = AZ_OK;
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pending_server_command->mutex));
  if (az_span_ptr(pending_server_command->correlation_id) != NULL)
  {
    // don't need to keep the mutex locked while we send the error response
    _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_server_command->mutex));

    // can add this command to a queue to be executed if the application supports executing
    // multiple commands at once.
    // printf(LOG_APP
    //         "Received command while another command is executing. Sending error response.\n");
    az_mqtt5_rpc_server_execution_rsp_event_data return_data
        = { .correlation_id = data.correlation_id,
            .error_message = AZ_SPAN_FROM_STR("Can't execute more than one command at a time"),
            .response_topic = data.response_topic,
            .request_topic = data.request_topic,
            .status = AZ_MQTT5_RPC_STATUS_THROTTLED,
            .response = AZ_SPAN_EMPTY,
            .content_type = AZ_SPAN_EMPTY };
    ret = az_mqtt5_rpc_server_execution_finish(rpc_server_policy, &return_data);
  }
  else
  {
    az_span_copy(pending_server_command->request_topic, data.request_topic);
    pending_server_command->request_topic
        = az_span_slice(pending_server_command->request_topic, 0, az_span_size(data.request_topic));
    az_span_copy(pending_server_command->response_topic, data.response_topic);
    az_span_copy(pending_server_command->request_data, data.request_data);
    az_span_copy(pending_server_command->content_type, data.content_type);
    pending_server_command->content_type
        = az_span_slice(pending_server_command->content_type, 0, az_span_size(data.content_type));
    pending_server_command->correlation_id = pending_server_command->correlation_id_buffer;
    az_span_copy(pending_server_command->correlation_id, data.correlation_id);
    pending_server_command->correlation_id
        = az_span_slice(pending_server_command->correlation_id, 0, az_span_size(data.correlation_id));
    pending_server_command->rpc_server_policy = rpc_server_policy;

    _az_RETURN_IF_FAILED(az_platform_timer_create(&pending_server_command->timer, timer_callback, pending_server_command));
    _az_RETURN_IF_FAILED(az_platform_timer_start(&pending_server_command->timer, delay_milliseconds));

    _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_server_command->mutex));
  }

  return ret;
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_pending_command_check_and_execute(
    az_mqtt5_rpc_server_pending_command* pending_server_command,
    az_span content_type,
    az_mqtt5_rpc_status (execute_command)(az_span request_data, az_span request_topic, az_span response)
    )
{
  az_result ret = AZ_OK;
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pending_server_command->mutex));
  if (az_span_ptr(pending_server_command->correlation_id) != NULL)
  {
    // copy correlation id to a new span so we can compare it later
    uint8_t copy_buffer[az_span_size(pending_server_command->correlation_id)];
    az_span correlation_id_copy
        = az_span_create(copy_buffer, az_span_size(pending_server_command->correlation_id));
    az_span_copy(correlation_id_copy, pending_server_command->correlation_id);

    if (!az_span_is_content_equal(content_type, pending_server_command->content_type))
    {
      // TODO: should this completely fail execution? This currently matches the C# implementation.
      // I feel like it should send an error response
      // printf(
      //     LOG_APP_ERROR "Invalid content type. Expected: {%s} Actual: {%s}\n",
      //     az_span_ptr(content_type),
      //     az_span_ptr(pending_server_command->content_type));
      _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_server_command->mutex));
      return AZ_ERROR_NOT_SUPPORTED;
    }

    az_mqtt5_rpc_status rc;
    // release mutex while command is executing since this could take a while
    _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_server_command->mutex));
    // figure out what should really be passed here
    rc = execute_command(pending_server_command->request_data, pending_server_command->request_topic, pending_server_command->response_payload_buffer);
    _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pending_server_command->mutex));

    // if command hasn't timed out, send result back
    if (az_span_is_content_equal(correlation_id_copy, pending_server_command->correlation_id))
    {
      _az_RETURN_IF_FAILED(az_platform_timer_destroy(&pending_server_command->timer));
      az_span response_payload = AZ_SPAN_EMPTY;
      az_span error_message = AZ_SPAN_EMPTY;
      if (rc == AZ_MQTT5_RPC_STATUS_OK)
      {
        response_payload = pending_server_command->response_payload_buffer;
      }
      else
      {
        error_message = pending_server_command->response_payload_buffer;
      }

      az_mqtt5_rpc_server_execution_rsp_event_data return_data
          = { .correlation_id = pending_server_command->correlation_id,
              .response = response_payload,
              .response_topic = pending_server_command->response_topic,
              .request_topic = pending_server_command->request_topic,
              .status = rc,
              .content_type = content_type,
              .error_message = error_message };
      _az_RETURN_IF_FAILED(
          az_mqtt5_rpc_server_execution_finish(pending_server_command->rpc_server_policy, &return_data));

      pending_server_command->content_type = pending_server_command->content_type_buffer;
      pending_server_command->request_topic = pending_server_command->request_topic_buffer;
      pending_server_command->correlation_id = AZ_SPAN_EMPTY;
    }
  }

  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_server_command->mutex));
  
  return ret;
}
