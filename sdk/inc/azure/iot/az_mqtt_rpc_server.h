// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Definition of #az_mqtt_rpc_server. You use the RPC server to receive commands.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_MQTT_RPC_SERVER_H
#define _az_MQTT_RPC_SERVER_H

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_common.h>
#include <azure/iot/az_iot_connection.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief The MQTT RPC Server.
 *
 */
typedef struct az_mqtt_rpc_server az_mqtt_rpc_server;

/**
 * @brief Event types for the MQTT RPC Server.
 *
 */
enum az_event_type_mqtt_rpc_server
{
  /**
   * @brief Event representing the application finishing processing the command.
   *
   */
  AZ_EVENT_MQTT_RPC_SERVER_EXECUTION_FINISH = _az_MAKE_EVENT(_az_FACILITY_IOT, 15),
  // AZ_MQTT_SERVER_EVENT_REGISTER_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT, 16),
  AZ_EVENT_RPC_SERVER_EXECUTE_COMMAND = _az_MAKE_EVENT(_az_FACILITY_IOT, 17),
};

typedef struct 
{
  az_span correlation_id;
  az_span response_topic;

} az_mqtt_rpc_server_pending_command;


/**
 * @brief MQTT RPC Server options.
 *
 */
typedef struct
{
  int8_t sub_qos;

  int8_t response_qos;

  az_span sub_topic;

  az_mqtt_rpc_server_pending_command pending_command;

  // name/type of command handled by this subclient (needed?)
  az_span command_handled;

  /**
   * @brief the message id of the pending subscribe for the command topic
  */
  int32_t _az_mqtt_rpc_server_pending_sub_id;

  /**
   * @brief The client id for the MQTT connection. REQUIRED if disable_sdk_connection_management is
   * false.
   *
   */
  // az_span client_id_buffer;


} az_mqtt_rpc_server_options;

/**
 * @brief The MQTT RPC Server.
 *
 */
struct az_mqtt_rpc_server
{
  struct
  {
    /**
     * @brief RPC Server policy for the MQTT RPC Server.
     *
     */
    _az_hfsm rpc_server_policy;

    _az_iot_subclient subclient;

    az_iot_connection* connection;

    /**
     * @brief Policy collection.
     *
     */
    // _az_event_policy_collection policy_collection;

    /**
     * @brief MQTT policy.
     *
     */
    // _az_mqtt_policy mqtt_policy;

    /**
     * @brief Event pipeline for the MQTT connection.
     *
     */
    // _az_event_pipeline event_pipeline;

    /**
     * @brief Options for the MQTT RPC Server.
     *
     */
    az_mqtt_rpc_server_options options;
  } _internal;
};

// Event data types
typedef struct az_mqtt_rpc_server_execution_data
{
  az_span correlation_id;
  az_span response_topic;
  int32_t status;
  az_span response;
  az_span error_message;
} az_mqtt_rpc_server_execution_data;

AZ_NODISCARD az_result az_mqtt_rpc_server_register(
    az_mqtt_rpc_server* client);

AZ_NODISCARD az_result _az_rpc_server_policy_init(_az_hfsm* hfsm,
    _az_iot_subclient* subclient,
    az_iot_connection* connection);

AZ_NODISCARD az_result az_rpc_server_init(
    az_mqtt_rpc_server* client,
    az_iot_connection* connection,
    az_mqtt_rpc_server_options* options);

AZ_NODISCARD az_result az_mqtt_rpc_server_execution_finish(
    az_mqtt_rpc_server* client,
    az_mqtt_rpc_server_execution_data* data);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_MQTT_RPC_SERVER_H
