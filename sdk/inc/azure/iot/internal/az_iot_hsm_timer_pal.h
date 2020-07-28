// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_hsm_timer_pal.h
 *
 * @brief Azure IoT Timer Platform Adaptation Layer
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_HSM_TIMER_PAL_INTERNAL_H
#define _az_IOT_HSM_TIMER_PAL_INTERNAL_H

#include <azure/core/az_result.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/internal/az_iot_hsm.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

AZ_NODISCARD az_result time_pal_start(int32_t ticks, az_iot_hsm* src);
AZ_INLINE az_result time_pal_signal(az_iot_hsm* dst)
{
  return az_iot_hsm_post_sync(dst, az_iot_hsm_timeout_event);
}

#endif //!_az_IOT_HSM_TIMER_PAL_INTERNAL_H
