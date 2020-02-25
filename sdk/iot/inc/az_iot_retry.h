// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_RETRY_H
#define _az_IOT_RETRY_H

#include "az_iot_status.h"
#include <az_result.h>
#include <stdint.h>

#include <_az_cfg_prefix.h>

// TODO: wait for guidance from AZ SDKs on facility & errors
#define AZ_RETRY_FACILITY 0x21
#define AZ_RETRY_NEEDED ((az_result)AZ_MAKE_ERROR(AZ_RETRY_FACILITY, 1))
#define AZ_RETRY_FAILED ((az_result)AZ_MAKE_ERROR(AZ_RETRY_FACILITY, 2))

typedef struct az_retry_context {
  struct {
    az_result last_error;
    uint32_t pow_seconds;
    uint16_t retry_count;
  } _internal;
} az_retry_context;

az_result az_retry_init(az_retry_context * context);
az_result az_retry_get_delay(
    az_retry_context * context,
    enum az_iot_status status,
    uint32_t random,
    uint32_t * retry_after);

#include <_az_cfg_suffix.h>

#endif _az_IOT_RETRY_H // !_az_IOT_RETRY_H
