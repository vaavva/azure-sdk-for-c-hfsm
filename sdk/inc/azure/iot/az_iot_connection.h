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

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_context.h>
#include <azure/core/az_credentials_x509.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/internal/az_event_pipeline.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct az_iot_connection az_iot_connection;

typedef az_result (*az_iot_connection_callback)(
    az_iot_connection* client,
    az_event event);

typedef struct
{
  bool disable_connection_management;
  az_credential_x509 secondary_credential;
} az_iot_connection_options;

struct az_iot_connection
{
  struct
  {
    // Pipeline and policies
    _az_event_pipeline pipeline;

    // Register operation
    az_iot_connection_callback event_callback;

    // Memory for generated fields
    az_span client_id_buffer;
    az_span username_buffer;
    az_span password_buffer;
  } _internal;
};

AZ_NODISCARD az_iot_connection_options az_iot_connection_options_default();

AZ_NODISCARD az_result az_iot_connection_init(
    az_iot_connection* client,
    az_context* context,
    az_mqtt* mqtt_client,
    az_credential_x509* primary_credential,
    char* client_id_buffer,
    size_t client_id_buffer_size,
    char* username_buffer,
    size_t username_buffer_size,
    char* password_buffer,
    size_t password_buffer_size,
    az_iot_connection_callback event_callback,
    az_iot_connection_options* options);

/// @brief Opens the connection to the broker.
/// @param client 
/// @return 
AZ_NODISCARD az_result az_iot_connection_open(az_iot_connection* client);

AZ_NODISCARD az_result az_iot_connection_close(az_iot_connection* client);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_IOT_CONNECTION
