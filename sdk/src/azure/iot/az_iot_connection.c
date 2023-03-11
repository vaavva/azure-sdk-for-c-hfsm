// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/iot/az_iot_connection.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_iot_connection_options az_iot_connection_options_default()
{
  return (az_iot_connection_options){ 0 };
}

AZ_NODISCARD az_result az_iot_connection_init(
    az_iot_connection* client,
    az_context* context,
    az_mqtt* mqtt_client,
    az_iot_connection_callback event_callback,
    az_iot_connection_options* options)
{
  _az_PRECONDITION_NOT_NULL(client);
  _az_PRECONDITION_NOT_NULL(context);
  _az_PRECONDITION_NOT_NULL(mqtt_client);
  _az_PRECONDITION_NOT_NULL(event_callback);

  client->_internal.options = options == NULL ? az_iot_connection_options_default() : *options;
  client->_internal.mqtt_client = mqtt_client;
  client->_internal.event_callback = event_callback;
  client->_internal.context = context;

  az_event_policy* outbound_event_policy = (az_event_policy*)&client->_internal.subclient_policy;
  az_event_policy* inbound_event_policy;

  if (client->_internal.options.connection_management)
  {
    // subclients_policy --> connection_policy --> az_mqtt

//TODO:    _az_RETURN_IF_FAILED(_az_hfsm_init(&client->_internal.connection_policy, root, _get_parent));
    // TODO: sent proper initial events

    inbound_event_policy = (az_event_policy*)&client->_internal.connection_policy;

// TODO    _az_RETURN_IF_FAILED(_az_iot_subclients_policy_init(
//        &client->_internal.subclient_policy, NULL, inbound_event_policy));
  }
  else
  {
    // subclients_policy --> az_mqtt
    inbound_event_policy = (az_event_policy*)&client->_internal.subclient_policy;

//TODO:    _az_RETURN_IF_FAILED(
//TODO:        _az_iot_subclients_policy_init(&client->_internal.subclient_policy, NULL, NULL));
  }

  _az_RETURN_IF_FAILED(_az_event_pipeline_init(
      &client->_internal.event_pipeline, outbound_event_policy, inbound_event_policy));

  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

/// @brief Opens the connection to the broker.
/// @param client
/// @return
AZ_NODISCARD az_result az_iot_connection_open(az_iot_connection* client)
{
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.event_pipeline, (az_event){ AZ_EVENT_IOT_CONNECTION_OPEN, NULL });
}

/// @brief Closes the connection to the broker. This will also close all subclients.
/// @param client
/// @return
AZ_NODISCARD az_result az_iot_connection_close(az_iot_connection* client)
{
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.event_pipeline, (az_event){ AZ_EVENT_IOT_CONNECTION_CLOSE, NULL });
}
