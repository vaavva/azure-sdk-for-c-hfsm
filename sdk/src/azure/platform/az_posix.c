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

void _thread_handler(union sigval sv);

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);

  // NOLINTNEXTLINE(bugprone-misplaced-widening-cast)
  *out_clock_msec = (int64_t)((time(NULL)) * _az_TIME_MILLISECONDS_PER_SECOND);

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

void _thread_handler(union sigval sv) 
{
  az_platform_timer *timer = sv.sival_ptr;

  _az_PRECONDITION_NOT_NULL(timer);
  _az_PRECONDITION_NOT_NULL(timer->_internal.callback);
  timer->_internal.callback(timer->_internal.sdk_data);
}

AZ_NODISCARD az_result az_platform_timer_create(
    az_platform_timer* timer,
    az_platform_timer_callback callback,
    void* sdk_data)
{
  _az_PRECONDITION_NOT_NULL(timer);
  memset(timer, 0, sizeof(az_platform_timer));

  timer->_internal.callback = callback;
  timer->_internal.sdk_data = sdk_data;

  timer->_internal.sev.sigev_notify = SIGEV_THREAD;
  timer->_internal.sev.sigev_notify_function = &_thread_handler;
  timer->_internal.sev.sigev_value.sival_ptr = timer;

  if (0 != timer_create(CLOCK_REALTIME, &timer->_internal.sev, &timer->_internal.timerid))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_timer_start(az_platform_timer* timer, int32_t milliseconds)
{
  timer->_internal.trigger.it_value.tv_sec = milliseconds / 1000;
  timer->_internal.trigger.it_value.tv_nsec = (milliseconds % 1000) * 1000000;

  if (0 != timer_settime(timer->_internal.timerid, 0, &timer->_internal.trigger, NULL))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_timer_destroy(az_platform_timer* timer)
{
  if (0 != timer_delete(timer->_internal.timerid))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_init(az_platform_mutex* mutex_handle)
{
  if (0 != pthread_mutex_init(mutex_handle, NULL))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_acquire(az_platform_mutex* mutex_handle)
{
  if (0 != pthread_mutex_lock(mutex_handle))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_release(az_platform_mutex* mutex_handle)
{
  if (0 != pthread_mutex_unlock(mutex_handle))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_destroy(az_platform_mutex* mutex_handle)
{
  if (0 != pthread_mutex_destroy(mutex_handle))
  {
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}
