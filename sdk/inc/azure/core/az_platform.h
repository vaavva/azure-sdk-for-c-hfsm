// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file
 *
 * @brief Defines platform-specific functionality used by the Azure SDK.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_PLATFORM_H
#define _az_PLATFORM_H

#include <azure/core/az_result.h>
#include <stdbool.h>
#include <stdint.h>
#if AZ_PLATFORM_IMPL == POSIX
#include "azure/platform/az_platform_posix.h"
#else
#include "azure/platform/az_platform_none.h"
#endif

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Gets the platform clock in milliseconds.
 *
 * @remark The moment of time where clock starts is undefined, but if this function is getting
 * called twice with one second interval, the difference between the values returned should be equal
 * to 1000.
 *
 * @param[out] out_clock_msec Platform clock in milliseconds.
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 */
AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec);

/**
 * @brief Tells the platform to sleep for a given number of milliseconds.
 *
 * @param[in] milliseconds Number of milliseconds to sleep.
 *
 * @remarks The behavior is undefined when \p milliseconds is a non-positive value (0 or less than
 * 0).
 *
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 */
AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds);

/**
 * @brief Called on critical error. This function should not return.
 *
 * @details In general, this function should cause the device to reboot or the main process to
 *          crash or exit.
 */
void az_platform_critical_error();

/**
 * @brief Gets a positive pseudo-random integer.
 *
 * @param[out] out_random A pseudo-random number greater than 0.
 *  *
 * @retval #AZ_OK Success.
 * @retval #AZ_ERROR_DEPENDENCY_NOT_PROVIDED No platform implementation was supplied to support this
 * function.
 *
 * @note This is NOT cryptographically secure.
 */
AZ_NODISCARD az_result az_platform_get_random(int32_t* out_random);

/**
 * @brief Create a timer object.
 *
 * @param[in] callback SDK callback to call when timer elapses.
 * @param[in] sdk_data SDK data associated with the timer.
 * @param[out] out_timer_handle The timer handle.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_platform_timer_create(
    az_platform_timer* timer,
    az_platform_timer_callback callback,
    void* sdk_data);

/**
 * @brief Starts the timer. This function can be called multiple times. The timer should call the
 *        callback at most once.
 *
 * @param[in] timer_handle The timer handle.
 * @param[in] milliseconds Time in milliseconds after which the platform must call the associated
 *                     #az_platform_timer_callback.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_platform_timer_start(az_platform_timer* timer, int32_t milliseconds);

/**
 * @brief Destroys a timer.
 *
 * @param[in] timer_handle The timer handle.
 */
AZ_NODISCARD az_result az_platform_timer_destroy(az_platform_timer* timer);

/**
 * @brief Creates a mutex.
 *
 * @param mutex The mutex handle.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_platform_mutex_init(az_platform_mutex* mutex_handle);
AZ_NODISCARD az_result az_platform_mutex_acquire(az_platform_mutex* mutex_handle);
AZ_NODISCARD az_result az_platform_mutex_release(az_platform_mutex* mutex_handle);
AZ_NODISCARD az_result az_platform_mutex_destroy(az_platform_mutex* mutex_handle);

#include <azure/core/_az_cfg_suffix.h>

#endif // _az_PLATFORM_H
