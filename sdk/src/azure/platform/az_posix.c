// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/internal/az_config_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);

  // NOLINTNEXTLINE(bugprone-misplaced-widening-cast)
  *out_clock_msec = (int64_t)((clock() / CLOCKS_PER_SEC) * _az_TIME_MILLISECONDS_PER_SECOND);

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds)
{
  (void)usleep((useconds_t)milliseconds * _az_TIME_MICROSECONDS_PER_MILLISECOND);
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_get_random(int32_t* out_random)
{
  _az_PRECONDITION_NOT_NULL(out_random);
  *out_random = (int32_t)random();
  return AZ_OK;
}

typedef struct 
{
  az_platform_timer_callback callback;
  void* sdk_data;
} _az_platform_posix_timer_data;

AZ_NODISCARD az_result az_platform_timer_create(
    az_platform_timer_callback callback,
    void* sdk_data,
    az_platform_timer* out_timer_handle)
{
  
}

AZ_NODISCARD az_result az_platform_timer_start(az_platform_timer timer_handle, int32_t milliseconds)
{

}

void az_platform_timer_destroy(az_platform_timer timer_handle)
{

}


AZ_NODISCARD az_result az_platform_mutex_create(az_platform_mutex* mutex_handle)
{

}

AZ_NODISCARD az_result az_platform_mutex_acquire(az_platform_mutex mutex_handle)
{

}

AZ_NODISCARD az_result az_platform_mutex_release(az_platform_mutex mutex_handle)
{

}

AZ_NODISCARD az_result az_platform_mutex_destroy(az_platform_mutex mutex_handle)
{

}
