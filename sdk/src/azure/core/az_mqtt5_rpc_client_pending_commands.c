// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_mqtt5_rpc_client.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result pending_commands_array_init(
    pending_commands_array* pending_commands,
    uint8_t correlation_id_buffers[RPC_CLIENT_MAX_PENDING_COMMANDS]
                                  [AZ_MQTT5_RPC_CORRELATION_ID_LENGTH])
{
  _az_RETURN_IF_FAILED(az_platform_mutex_init(&pending_commands->mutex));
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pending_commands->mutex));
  pending_commands->pending_commands_count = 0;
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    pending_commands->commands[i].correlation_id
        = az_span_create(correlation_id_buffers[i], AZ_MQTT5_RPC_CORRELATION_ID_LENGTH);
    az_span_fill(pending_commands->commands[i].correlation_id, 0x0);
    pending_commands->commands[i].command_name = AZ_SPAN_EMPTY;
  }
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_commands->mutex));
  return AZ_OK;
}

AZ_NODISCARD az_result add_command(
    pending_commands_array* pending_commands,
    az_span correlation_id,
    az_span command_name,
    int32_t timeout_ms)
{
  az_result ret = AZ_OK;
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pending_commands->mutex));
  if (pending_commands->pending_commands_count >= RPC_CLIENT_MAX_PENDING_COMMANDS)
  {
    _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_commands->mutex));
    return AZ_ERROR_OUT_OF_MEMORY;
  }
  int32_t empty_index = -1;
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (az_span_size(pending_commands->commands[i].command_name) == 0)
    {
      empty_index = i;
      break;
    }
  }
  if (empty_index < 0)
  {
    _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_commands->mutex));
    return AZ_ERROR_OUT_OF_MEMORY;
  }

  pending_commands->pending_commands_count++;
  az_span_copy(pending_commands->commands[empty_index].correlation_id, correlation_id);
  pending_commands->commands[empty_index].command_name = command_name;

  int64_t clock = 0;
  ret = az_platform_clock_msec(&clock);
  pending_commands->commands[empty_index].context
      = az_context_create_with_expiration(&az_context_application, clock + timeout_ms);
  
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_commands->mutex));
  return ret;
}

az_result remove_command(pending_commands_array* pending_commands, az_span correlation_id)
{
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pending_commands->mutex));
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (az_span_is_content_equal(pending_commands->commands[i].correlation_id, correlation_id))
    {
      az_context_cancel(&pending_commands->commands[i].context);
      az_span_fill(pending_commands->commands[i].correlation_id, 0x0);
      pending_commands->commands[i].command_name = AZ_SPAN_EMPTY;
      pending_commands->pending_commands_count--;
      _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_commands->mutex));
      return AZ_OK;
    }
  }
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pending_commands->mutex));
  return AZ_ERROR_ITEM_NOT_FOUND;
}

AZ_NODISCARD bool is_command_pending(pending_commands_array pending_commands, az_span correlation_id)
{
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (az_span_is_content_equal(pending_commands.commands[i].correlation_id, correlation_id))
    {
      return true;
    }
  }
  return false;
}

/**
 * @brief Get the first expired command from the pending commands array.
 *
 */
AZ_NODISCARD pending_command* get_first_expired_command(pending_commands_array pending_commands)
{
  pending_command* expired_command = NULL;
  int64_t current_time = 0;
  if (az_result_failed(az_platform_clock_msec(&current_time)))
  {
    return NULL;
  }
  for (int i = 0; i < RPC_CLIENT_MAX_PENDING_COMMANDS; i++)
  {
    if (az_span_size(pending_commands.commands[i].command_name) > 0
        && az_context_has_expired(&pending_commands.commands[i].context, current_time))
    {
      if (expired_command == NULL
          || az_context_get_expiration(&pending_commands.commands[i].context)
              < az_context_get_expiration(&expired_command->context))
      {
        // Doesn't break here because we want to make sure we remove the most expired command first
        expired_command = &pending_commands.commands[i];
      }
    }
  }
  return expired_command;
}
