// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
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
    az_credential_x509* primary_credential,
    char* client_id_buffer,
    size_t client_id_buffer_size,
    char* username_buffer,
    size_t username_buffer_size,
    char* password_buffer,
    size_t password_buffer_size,
    az_iot_connection_callback event_callback,
    az_iot_connection_options* options)
{
    return AZ_ERROR_NOT_IMPLEMENTED;
}

/// @brief Opens the connection to the broker.
/// @param client 
/// @return 
AZ_NODISCARD az_result az_iot_connection_open(az_iot_connection* client)
{
    return AZ_ERROR_NOT_IMPLEMENTED;
}

AZ_NODISCARD az_result az_iot_connection_close(az_iot_connection* client)
{
    return AZ_ERROR_NOT_IMPLEMENTED;
}
