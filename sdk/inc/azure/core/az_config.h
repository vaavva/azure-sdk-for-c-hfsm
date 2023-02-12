// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_config.h
 *
 * @brief Configurable constants used by the Azure SDK.
 *
 * @remarks Typically, these constants do not need to be modified but depending on how your
 * application uses an Azure service, they can be adjusted.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_CONFIG_H
#define _az_CONFIG_H

#include <azure/core/_az_cfg_prefix.h>

enum
{
  /// The maximum buffer size for a URL.
  AZ_HTTP_REQUEST_URL_BUFFER_SIZE = 2 * 1024,

  /// The maximum buffer size for an HTTP request body.
  AZ_HTTP_REQUEST_BODY_BUFFER_SIZE = 1024,

  /// The maximum buffer size for a log message.
  AZ_LOG_MESSAGE_BUFFER_SIZE = 1024,

  /// The MQTT keepalive in seconds.
  AZ_MQTT_KEEPALIVE_SECONDS = 240,

  /// Maximum time in seconds that #az_mqtt_synchronous_process_loop should block.
  AZ_MQTT_SYNC_MAX_POLLING_MILLISECONDS = 500,

  /// SAS Token Lifetime in minutes
  AZ_IOT_SAS_TOKEN_LIFETIME_MINUTES = 60,

  // DPS Minimum Retry in seconds.
  AZ_IOT_PROVISIONING_RETRY_MINIMUM_TIMEOUT_SECONDS = 3,

  /// MQTT Maximum topic size in bytes.
  AZ_IOT_MAX_TOPIC_SIZE = 128,
  
  AZ_IOT_MAX_PAYLOAD_SIZE = 256,

  AZ_IOT_MAX_USERNAME_SIZE = 256,

  AZ_IOT_MAX_PASSWORD_SIZE = 128,

  AZ_IOT_MAX_CLIENT_ID_SIZE = 64,

  AZ_IOT_MAX_HUB_NAME_SIZE = 128,

  // C-SDK Compat Layer maximum number of queued messages.
  AZ_IOT_COMPAT_CSDK_MAX_QUEUE_SIZE = 10,

  AZ_IOT_MQTT_DISCONNECT_MS = 100,
};

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_CONFIG_H
