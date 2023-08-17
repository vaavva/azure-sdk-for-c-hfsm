// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt5.h>
#include <azure/core/az_mqtt5_rpc_server.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <stdio.h>
#include <stdlib.h>

#include "mqtt_protocol.h"

#include <azure/core/_az_cfg.h>

static az_result root(az_event_policy* me, az_event event);
static az_result waiting(az_event_policy* me, az_event event);
static az_result faulted(az_event_policy* me, az_event event);

AZ_NODISCARD az_result _az_rpc_server_policy_init(
    _az_hfsm* hfsm,
    _az_event_client* event_client,
    az_mqtt5_connection* connection);

static az_event_policy_handler _get_parent(az_event_policy_handler child_state)
{
  az_event_policy_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == waiting || child_state == faulted)
  {
    parent_state = root;
  }
  else
  {
    // Unknown state.
    az_platform_critical_error();
    parent_state = NULL;
  }

  return parent_state;
}

static az_result root(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_server"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;

    case AZ_HFSM_EVENT_ERROR:
    {
      if (az_result_failed(az_event_policy_send_inbound_event(me, event)))
      {
        az_platform_critical_error();
      }
      break;
    }

    case AZ_HFSM_EVENT_EXIT:
    {
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_EXIT))
      {
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_mqtt5_rpc_server_hfsm: PANIC!"));
      }

      az_platform_critical_error();
      break;
    }

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_CLOSE_REQ:
    case AZ_MQTT5_EVENT_DISCONNECT_RSP:
      break;

    default:
      // TODO
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

/**
 * @brief start subscription timer
 */
AZ_INLINE az_result _rpc_start_timer(az_mqtt5_rpc_server_hfsm* me)
{
  _az_event_pipeline* pipeline = &me->_internal.connection->_internal.event_pipeline;
  _az_event_pipeline_timer* timer = &me->_internal.rpc_server_timer;

  _az_RETURN_IF_FAILED(_az_event_pipeline_timer_create(pipeline, timer));

  int32_t delay_milliseconds = (int32_t)me->_internal.rpc_server->options.subscribe_timeout_in_seconds * 1000;

  _az_RETURN_IF_FAILED(az_platform_timer_start(&timer->platform_timer, delay_milliseconds));

  return AZ_OK;
}

/**
 * @brief stop subscription timer
 */
AZ_INLINE az_result _rpc_stop_timer(az_mqtt5_rpc_server_hfsm* me)
{
  _az_event_pipeline_timer* timer = &me->_internal.rpc_server_timer;
  return az_platform_timer_destroy(&timer->platform_timer);
}

/**
 * @brief helper function to check if an az_span topic matches an az_span subscription
 */
AZ_NODISCARD AZ_INLINE bool az_span_topic_matches_sub(az_span sub, az_span topic)
{
  bool ret;
  // TODO: have this not be mosquitto specific
  if (MOSQ_ERR_SUCCESS
      != mosquitto_topic_matches_sub((char*)az_span_ptr(sub), (char*)az_span_ptr(topic), &ret))
  {
    ret = false;
  }
  return ret;
}

/**
 * @brief Send a response publish and clear the pending command
 *
 * @param me
 * @param data event data for a publish request
 *
 */
AZ_INLINE az_result _send_response_pub(az_mqtt5_rpc_server_hfsm* me, az_mqtt5_pub_data data)
{
  az_result ret = AZ_OK;
  // send publish
  ret = az_event_policy_send_outbound_event(
      (az_event_policy*)me, (az_event){ .type = AZ_MQTT5_EVENT_PUB_REQ, .data = &data });

  // empty the property bag so it can be reused
  _az_RETURN_IF_FAILED(az_mqtt5_property_bag_empty(&me->_internal.rpc_server->property_bag));
  return ret;
}

/**
 * @brief Main state where the rpc server waits for incoming command requests or execution to
 * complete
 */
static az_result waiting(az_event_policy* me, az_event event)
{
  az_result ret = AZ_OK;
  az_mqtt5_rpc_server_hfsm* this_policy = (az_mqtt5_rpc_server_hfsm*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_server/waiting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op
      break;

    case AZ_MQTT5_EVENT_SUBACK_RSP:
    {
      // if get suback that matches the sub we sent, stop waiting for the suback
      az_mqtt5_suback_data* data = (az_mqtt5_suback_data*)event.data;
      if (data->id == this_policy->_internal.pending_subscription_id)
      {
        _rpc_stop_timer(this_policy);
        this_policy->_internal.pending_subscription_id = 0;
      }
      // else, keep waiting for the proper suback
      break;
    }

    case AZ_HFSM_EVENT_TIMEOUT:
    {
      if (event.data == &this_policy->_internal.rpc_server_timer)
      {
        // if subscribing times out, go to faulted state - this is not recoverable
        _az_RETURN_IF_FAILED(_az_hfsm_transition_peer((_az_hfsm*)me, waiting, faulted));
      }
      break;
    }

    case AZ_MQTT5_EVENT_PUB_RECV_IND:
    {
      az_mqtt5_recv_data* recv_data = (az_mqtt5_recv_data*)event.data;
      // Ensure pub is of the right topic
      if (az_span_topic_matches_sub(this_policy->_internal.rpc_server->subscription_topic, recv_data->topic))
      {
        // clear subscription timer if we get a pub on the topic, since that implies we're
        // subscribed
        if (this_policy->_internal.pending_subscription_id != 0)
        {
          _rpc_stop_timer(this_policy);
          this_policy->_internal.pending_subscription_id = 0;
        }

        // parse the request details
        az_mqtt5_rpc_server_execution_req_event_data command_data;
        az_mqtt5_rpc_server_property_pointers props_to_free;
        _az_RETURN_IF_FAILED(az_rpc_server_parse_request_topic_and_properties(this_policy->_internal.rpc_server, recv_data, &props_to_free, &command_data.request_data));
        
        // send to application for execution
        // if ((az_event_policy*)this_policy->inbound_policy != NULL)
        // {
        // az_event_policy_send_inbound_event((az_event_policy*)this_policy, (az_event){.type =
        // AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ, .data = data});
        // }
        _az_RETURN_IF_FAILED(_az_mqtt5_connection_api_callback(
            this_policy->_internal.connection,
            (az_event){ .type = AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_REQ, .data = &command_data }));

        az_rpc_server_free_properties(props_to_free);
      }
      break;
    }

    case AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_RSP:
    {
      az_mqtt5_rpc_server_execution_rsp_event_data* event_data
          = (az_mqtt5_rpc_server_execution_rsp_event_data*)event.data;

      // Check that original request topic matches the subscription topic for this RPC server
      // instance
      if (az_span_topic_matches_sub(
              this_policy->_internal.rpc_server->subscription_topic, event_data->request_topic))
      {
        // create response payload
        az_mqtt5_pub_data data;
        _az_RETURN_IF_FAILED(az_rpc_server_get_response_packet(this_policy->_internal.rpc_server, &event_data->response_data, &data));

        // send publish
        _send_response_pub(this_policy, data);
      }
      else
      {
        // log and ignore (this is probably meant for a different policy)
        printf("topic does not match subscription, ignoring\n");
      }
      break;
    }

    case AZ_MQTT5_EVENT_PUBACK_RSP:
    case AZ_EVENT_MQTT5_CONNECTION_OPEN_REQ:
    case AZ_MQTT5_EVENT_CONNECT_RSP:
      break;

    case AZ_HFSM_EVENT_EXIT:
      // No-op
      break;

    default:
      // TODO
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

/**
 * @brief Failure state - locks up all execution of this hfsm
 */
static az_result faulted(az_event_policy* me, az_event event)
{
  az_result ret = AZ_ERROR_HFSM_INVALID_STATE;
  (void)me;
#ifdef AZ_NO_LOGGING
  (void)event;
#endif // AZ_NO_LOGGING

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_rpc_server/faulted"));
  }

  return ret;
}

AZ_NODISCARD az_result _az_rpc_server_policy_init(
    _az_hfsm* hfsm,
    _az_event_client* event_client,
    az_mqtt5_connection* connection)
{
  _az_RETURN_IF_FAILED(_az_hfsm_init(hfsm, root, _get_parent, NULL, NULL));
  _az_RETURN_IF_FAILED(_az_hfsm_transition_substate(hfsm, root, waiting));

  event_client->policy = (az_event_policy*)hfsm;
  _az_RETURN_IF_FAILED(_az_event_policy_collection_add_client(
      &connection->_internal.policy_collection, event_client));

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_hfsm_register(az_mqtt5_rpc_server_hfsm* client)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  az_mqtt5_sub_data subscription_data = { .topic_filter = client->_internal.rpc_server->subscription_topic,
                                          .qos = client->_internal.rpc_server->options.subscribe_qos,
                                          .out_id = 0 };
  _rpc_start_timer(client);
  _az_RETURN_IF_FAILED(az_event_policy_send_outbound_event(
      (az_event_policy*)client,
      (az_event){ .type = AZ_MQTT5_EVENT_SUB_REQ, .data = &subscription_data }));
  client->_internal.pending_subscription_id = subscription_data.out_id;
  return AZ_OK;
}

AZ_NODISCARD az_result az_rpc_server_hfsm_init(
    az_mqtt5_rpc_server_hfsm* client,
    az_mqtt5_connection* connection,
    az_mqtt5_property_bag property_bag,
    az_span subscription_topic,
    az_span model_id,
    az_span client_id,
    az_span command_name,
    az_mqtt5_rpc_server_options* options)
{
  _az_RETURN_IF_FAILED(az_rpc_server_init(client->_internal.rpc_server, property_bag, model_id, client_id, command_name, subscription_topic, options));

  client->_internal.connection = connection;

  // Initialize the stateful sub-client.
  if ((connection != NULL))
  {
    _az_RETURN_IF_FAILED(
        _az_rpc_server_policy_init((_az_hfsm*)client, &client->_internal.subclient, connection));
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_mqtt5_rpc_server_execution_finish(
    az_mqtt5_rpc_server_hfsm* client,
    az_mqtt5_rpc_server_execution_rsp_event_data* data)
{
  if (client->_internal.connection == NULL)
  {
    // This API can be called only when the client is attached to a connection object.
    return AZ_ERROR_NOT_SUPPORTED;
  }

  _az_PRECONDITION_VALID_SPAN(data->response_data.correlation_id, 1, false);
  _az_PRECONDITION_VALID_SPAN(data->response_data.response_topic, 1, false);
  // _az_PRECONDITION_VALID_SPAN(data->response, 0, true);
  // _az_PRECONDITION_VALID_SPAN(data->error_message, 0, true);

  return _az_event_pipeline_post_outbound_event(
      &client->_internal.connection->_internal.event_pipeline,
      (az_event){ .type = AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND_RSP, .data = data });
}
