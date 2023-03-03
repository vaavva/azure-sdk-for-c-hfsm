// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @brief Timer callback.
 *
 * @param[in] sdk_data Data passed by the SDK during the #az_platform_timer_create call.
 */
typedef void (*az_platform_timer_callback)(void* sdk_data);

typedef struct
{
  struct
  {
    az_platform_timer_callback callback;
    void* sdk_data;
  } _internal;
} az_platform_common;

