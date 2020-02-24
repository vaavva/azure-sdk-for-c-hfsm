// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_IOT_STATUS_H
#define _az_IOT_STATUS_H

#include <_az_cfg_prefix.h>

enum az_iot_status
{
    // missing success cases.
    AZ_IOT_STATUS_BAD_REQUEST = 400,
    AZ_IOT_STATUS_UNAUTHORIZED = 401,
    AZ_IOT_STATUS_FORBIDDEN = 403,
    AZ_IOT_STATUS_NOT_FOUND = 404,
    AZ_IOT_STATUS_NOT_ALLOWED = 405,
    AZ_IOT_STATUS_NOT_CONFLICT = 409,
    AZ_IOT_STATUS_PRECONDITION_FAILED = 412,
    AZ_IOT_STATUS_REQUEST_TOO_LARGE = 413,
    AZ_IOT_STATUS_UNSUPPORTED_TYPE = 415,
    AZ_IOT_STATUS_THROTTLED = 429,
    AZ_IOT_STATUS_CLIENT_CLOSED = 499,
    AZ_IOT_STATUS_SERVER_ERROR = 500,
    AZ_IOT_STATUS_BAD_GATEWAY = 502,
    AZ_IOT_STATUS_SERVICE_UNAVAILABLE = 503,
    AZ_IOT_STATUS_TIMEOUT = 504,
};

#include <_az_cfg_suffix.h>

#endif _az_IOT_STATUS_H // !_az_IOT_STATUS_H
