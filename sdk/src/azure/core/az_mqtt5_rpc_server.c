// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5_rpc_server.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/_az_cfg.h>

/**
 * @brief Handle an incoming request
 *
 * @param this_policy
 * @param data event data received from the publish
 * 
 * @note MUST call az_rpc_server_free_properties on props_to_free after copying what's needed from the request
 *
 * @return az_result
 */
AZ_NODISCARD az_result az_rpc_server_parse_request_topic_and_properties(az_mqtt5_rpc_server* client, az_mqtt5_recv_data* data, az_mqtt5_rpc_server_property_pointers* props_to_free, az_mqtt5_rpc_server_execution_req_event_data* out_request)
{
  _az_PRECONDITION_NOT_NULL(data->properties);
  (void)client;

  // save the response topic
  // az_mqtt5_property_string response_topic;
  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_string_read(
      data->properties, AZ_MQTT5_PROPERTY_TYPE_RESPONSE_TOPIC, &props_to_free->_internal.response_topic));

  // save the correlation data to send back with the response
  // az_mqtt5_property_binarydata correlation_data;
  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_binarydata_read(
      data->properties, AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA, &props_to_free->_internal.correlation_data));

  // validate request isn't expired?

  // read the content type so the application can properly deserialize the request
  // az_mqtt5_property_string content_type;
  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_string_read(
      data->properties, AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE, &props_to_free->_internal.content_type));

  out_request->correlation_id = az_mqtt5_property_binarydata_get(&props_to_free->_internal.correlation_data);
  out_request->response_topic = az_mqtt5_property_string_get(&props_to_free->_internal.response_topic);
  out_request->request_data = data->payload;
  out_request->request_topic = data->topic;
  out_request->content_type = az_mqtt5_property_string_get(&props_to_free->_internal.content_type);

  return AZ_OK;
}

void az_rpc_server_free_properties(az_mqtt5_rpc_server_property_pointers props)
{
  az_mqtt5_property_string_free(&props._internal.content_type);
  az_mqtt5_property_binarydata_free(&props._internal.correlation_data);
  az_mqtt5_property_string_free(&props._internal.response_topic);
}

az_result az_rpc_server_empty_property_bag(az_mqtt5_rpc_server* client)
{
  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_empty(&client->_internal.property_bag));

  return AZ_OK;
}

/**
 * @brief Build the reponse payload given the execution finish data
 *
 * @param me
 * @param event_data execution finish data
 *    contains status code, and error message or response payload
 * @param out_data event data for response publish
 * @return az_result
 */
AZ_NODISCARD az_result az_rpc_server_get_response_packet(
    az_mqtt5_rpc_server* client,
    az_mqtt5_rpc_server_execution_rsp_event_data* event_data,
    az_mqtt5_pub_data* out_data)
{

  // if the status indicates failure, add the status message to the user properties
  if (event_data->status < 200 || event_data->status >= 300)
  {
    // TODO: is an error message required on failure?
    _az_PRECONDITION_VALID_SPAN(event_data->error_message, 0, true);
    az_mqtt5_property_stringpair status_message_property
        = { .key = AZ_SPAN_FROM_STR("statusMessage"), .value = event_data->error_message };

    _az_RETURN_IF_FAILED(az_mqtt5_property_bag_stringpair_append(
        &client->_internal.property_bag,
        AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
        &status_message_property));
    out_data->payload = AZ_SPAN_EMPTY;
  }
  // if the status indicates success, add the response payload to the publish and set the content
  // type property
  else
  {
    // TODO: is a payload required?
    _az_PRECONDITION_VALID_SPAN(event_data->response, 0, true);
    az_mqtt5_property_string content_type = { .str = event_data->content_type };

    _az_RETURN_IF_FAILED(az_mqtt5_property_bag_string_append(
        &client->_internal.property_bag, AZ_MQTT5_PROPERTY_TYPE_CONTENT_TYPE, &content_type));

    out_data->payload = event_data->response;
  }

  // Set the status user property
  char status_str[5];
  sprintf(status_str, "%d", event_data->status);
  az_mqtt5_property_stringpair status_property
      = { .key = AZ_SPAN_FROM_STR("status"), .value = az_span_create_from_str(status_str) };

  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_stringpair_append(
      &client->_internal.property_bag,
      AZ_MQTT5_PROPERTY_TYPE_USER_PROPERTY,
      &status_property));

  // Set the correlation data property
  _az_PRECONDITION_VALID_SPAN(event_data->correlation_id, 0, true);
  az_mqtt5_property_binarydata correlation_data = { .bindata = event_data->correlation_id };
  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_binary_append(
      &client->_internal.property_bag,
      AZ_MQTT5_PROPERTY_TYPE_CORRELATION_DATA,
      &correlation_data));

  out_data->properties = &client->_internal.property_bag;
  // use the received response topic as the topic
  out_data->topic = event_data->response_topic;
  out_data->qos = client->_internal.options.response_qos;

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
    az_mqtt5_property_bag property_bag,
    az_span model_id, az_span client_id, az_span command_name,
    az_span subscription_topic,
    az_mqtt5_rpc_server_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  client->_internal.options = options == NULL ? az_mqtt5_rpc_server_options_default() : *options;

  _az_RETURN_IF_FAILED(az_rpc_server_get_subscription_topic(client, model_id, client_id, command_name, subscription_topic));

  client->_internal.subscription_topic = subscription_topic;

  client->_internal.property_bag = property_bag;

  return AZ_OK;
}