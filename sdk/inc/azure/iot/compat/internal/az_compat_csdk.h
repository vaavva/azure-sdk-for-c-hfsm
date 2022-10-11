// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_csdk_compat.h
 * 
 * @brief Defines the CSDK compat layer.
 * 
 */


#ifndef _az_COMPAT_CSDK
#define _az_COMPAT_CSDK

#include <azure/core/az_log.h>

#include <azure/core/_az_cfg_prefix.h>

#define LOG_COMPAT "\x1B[34mCOMPAT: \x1B[0m"
#define LOG_SDK "\x1B[33mSDK: \x1B[0m"

const char* az_result_string(az_result result);
void az_sdk_log_callback(az_log_classification classification, az_span message);
bool az_sdk_log_filter_callback(az_log_classification classification);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_COMPAT_CSDK
