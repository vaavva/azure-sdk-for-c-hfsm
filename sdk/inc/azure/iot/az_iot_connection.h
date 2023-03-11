// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_connection.h
 *
 * @brief Definition for the Azure Device Provisioning client state machine.
 * @remark The Device Provisioning MQTT protocol is described at
 * https://docs.microsoft.com/azure/iot-dps/iot-dps-mqtt-support
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_CONNECTION
#define _az_IOT_CONNECTION

#include <azure/core/az_context.h>
#include <azure/core/az_credentials_x509.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_event_pipeline.h>
#include <azure/iot/internal/az_iot_subclients_policy.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

enum az_event_type_iot_connection
{
  AZ_EVENT_IOT_CONNECTION_OPEN_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT, 10),

  AZ_EVENT_IOT_CONNECTION_CLOSE_REQ = _az_MAKE_EVENT(_az_FACILITY_IOT, 11),
};

typedef struct az_iot_connection az_iot_connection;

typedef az_result (*az_iot_connection_callback)(az_iot_connection* client, az_event event);

typedef struct
{
  // HFSM_DESIGN Leave default to have all the values filled in by the first sub-client.
  az_span hostname;
  int16_t port;

  bool connection_management;

  // HFSM_DESIGN The following settings and buffers are required if connection_management is true.
  //             The first sub-client attached is responsible with filling in the values.
  //             If we want to further optimize, we can convert connection_management to
  //             a preprocessor statement.
  az_span client_id_buffer;
  az_span username_buffer;
  az_span password_buffer;
  az_credential_x509* primary_credential;
  az_credential_x509* secondary_credential;
} az_iot_connection_options;

struct az_iot_connection
{
  struct
  {
    _az_hfsm connection_policy;
    _az_iot_subclients_policy subclient_policy;

    _az_event_pipeline event_pipeline;

    az_mqtt* mqtt_client;
    az_context* context;
    az_iot_connection_callback event_callback;

    az_iot_connection_options options;
  } _internal;
};

AZ_NODISCARD az_iot_connection_options az_iot_connection_options_default();

AZ_NODISCARD az_result az_iot_connection_init(
    az_iot_connection* client,
    az_context* context,
    az_mqtt* mqtt_client,
    az_iot_connection_callback event_callback,
    az_iot_connection_options* options);

/// @brief Opens the connection to the broker.
/// @param client
/// @return
AZ_INLINE az_result az_iot_connection_open(az_iot_connection* client)
{
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.event_pipeline, (az_event){ AZ_EVENT_IOT_CONNECTION_OPEN_REQ, NULL });
}

AZ_INLINE az_result az_iot_connection_close(az_iot_connection* client)
{
  return _az_event_pipeline_post_outbound_event(
      &client->_internal.event_pipeline, (az_event){ AZ_EVENT_IOT_CONNECTION_CLOSE_REQ, NULL });
}

AZ_INLINE az_result _az_iot_connection_api_callback(az_iot_connection* client, az_event event)
{
  return client->_internal.event_callback(client, event);
}

AZ_NODISCARD az_result
_az_iot_connection_policy_init(_az_hfsm* hfsm, az_event_policy* outbound, az_event_policy* inbound);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_CONNECTION
