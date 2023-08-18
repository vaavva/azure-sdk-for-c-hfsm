// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc_server.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_rpc_server_parse_request_topic(az_mqtt5_rpc_server* client, az_span request_topic, az_mqtt5_rpc_server_command_request_specification* out_request)
{
  _az_PRECONDITION_VALID_SPAN(request_topic, 0, false);
  (void)client;

  //TODO: parse properly
  out_request->model_id = az_span_create_from_str("dtmi:rpc:samples:vehicle;1");
  out_request->executor_client_id = az_span_create_from_str("vehicle03");
  out_request->command_name = az_span_create_from_str("unlock");
  out_request->invoker_client_id = AZ_SPAN_EMPTY;

  return AZ_OK;
}


AZ_NODISCARD az_result az_rpc_server_get_subscription_topic(az_mqtt5_rpc_server* client, az_span model_id, az_span client_id, az_span command_name, az_span out_subscription_topic)
{
  (void)client;
#ifndef AZ_NO_PRECONDITION_CHECKING
  _az_PRECONDITION_VALID_SPAN(model_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(client_id, 1, false);
  int32_t subscription_min_length = az_span_size(model_id) + az_span_size(client_id)
      + (az_span_size(command_name) > 0 ? az_span_size(command_name) : 1) + 23;
  _az_PRECONDITION_VALID_SPAN(out_subscription_topic, subscription_min_length, true);
#endif

  az_span temp_span = out_subscription_topic;
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("vehicles/"));
  temp_span = az_span_copy(temp_span, model_id);
  temp_span = az_span_copy(temp_span, AZ_SPAN_FROM_STR("/commands/"));
  temp_span = az_span_copy(temp_span, client_id);
  temp_span = az_span_copy_u8(temp_span, '/');
  temp_span = az_span_copy(temp_span, _az_span_is_valid(command_name, 1, 0) ? command_name : AZ_SPAN_FROM_STR("+"));
  temp_span = az_span_copy_u8(temp_span, '\0');

  return AZ_OK;
}

AZ_NODISCARD az_mqtt5_rpc_server_options az_mqtt5_rpc_server_options_default()
{
  return (az_mqtt5_rpc_server_options){ .subscribe_qos = AZ_MQTT5_RPC_QOS,
                                        .response_qos = AZ_MQTT5_RPC_QOS,
                                        .subscribe_timeout_in_seconds
                                        = AZ_MQTT5_RPC_SERVER_DEFAULT_TIMEOUT_SECONDS };
}

AZ_NODISCARD az_result az_rpc_server_init(
    az_mqtt5_rpc_server* client,
    az_span model_id, az_span client_id, az_span command_name,
    az_span subscription_topic,
    az_mqtt5_rpc_server_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  client->options = options == NULL ? az_mqtt5_rpc_server_options_default() : *options;

  _az_RETURN_IF_FAILED(az_rpc_server_get_subscription_topic(client, model_id, client_id, command_name, subscription_topic));

  client->subscription_topic = subscription_topic;

  return AZ_OK;
}