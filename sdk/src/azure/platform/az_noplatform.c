// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);
  *out_clock_msec = 0;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds)
{
  (void)milliseconds;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

void az_platform_critical_error()
{
  while (true) {}
}

AZ_NODISCARD az_result az_platform_get_random(int32_t* out_random)
{
  (void)random;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_timer_create(
    az_platform_timer_callback callback,
    void* sdk_data,
    void** out_timer_handle)
{
  (void)callback;
  (void)sdk_data;
  (void)out_timer_handle;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_timer_start(void* timer_handle, int32_t milliseconds)
{
  (void)timer_handle;
  (void)milliseconds;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

void az_platform_timer_destroy(void* timer_handle)
{
  (void)timer_handle;
  // no-op
}


AZ_NODISCARD az_result az_platform_queue_push(void* queue_handle, void const* element)
{
  (void)queue_handle;
  (void)element;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}

AZ_NODISCARD az_result az_platform_queue_pop(void* queue_handle, void** out_element)
{
  (void)queue_handle;
  (void)element;
  return AZ_ERROR_DEPENDENCY_NOT_PROVIDED;
}
