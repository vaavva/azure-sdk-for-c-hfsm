// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

static az_platform_mutex* test_mock_mutex = NULL;
static int test_counter_mutex = -1;

static az_platform_timer* test_mock_timer = NULL;

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);
  *out_clock_msec = (int64_t)mock();
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds)
{
  (void)milliseconds;
  return AZ_OK;
}

void az_platform_critical_error()
{
  assert_true(0);
}

AZ_NODISCARD az_result az_platform_get_random(int32_t* out_random)
{
  _az_PRECONDITION_NOT_NULL(out_random);
  *out_random = (int32_t)mock();
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_timer_create(
    az_platform_timer* timer_handle,
    az_platform_timer_callback callback,
    void* sdk_data)
{
  _az_PRECONDITION_NOT_NULL(timer_handle);

  test_mock_timer = timer_handle;
  timer_handle->_internal.callback = callback;
  timer_handle->_internal.sdk_data = sdk_data;

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_timer_start(
    az_platform_timer* timer_handle,
    int32_t milliseconds)
{
  _az_PRECONDITION_NOT_NULL(timer_handle);
  (void) milliseconds;

  assert_ptr_equal(timer_handle, test_mock_timer);

  return AZ_OK;
}

AZ_NODISCARD az_result  az_platform_timer_destroy(az_platform_timer* timer_handle)
{
  _az_PRECONDITION_NOT_NULL(timer_handle);

  assert_ptr_equal(timer_handle, test_mock_timer);
  timer_handle = NULL;

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_init(az_platform_mutex* mutex_handle)
{
  _az_PRECONDITION_NOT_NULL(mutex_handle);

  test_mock_mutex = mutex_handle;

  assert_int_equal(test_counter_mutex, -1); // Checking mutex is uninitialized.
  test_counter_mutex++;

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_acquire(az_platform_mutex* mutex_handle)
{
  _az_PRECONDITION_NOT_NULL(mutex_handle);

  assert_ptr_equal(mutex_handle, test_mock_mutex);
  assert_int_equal(test_counter_mutex, 0);
  test_counter_mutex++;

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_release(az_platform_mutex* mutex_handle)
{
  _az_PRECONDITION_NOT_NULL(mutex_handle);

  assert_ptr_equal(mutex_handle, test_mock_mutex);
  assert_int_equal(test_counter_mutex, 1);
  test_counter_mutex--;

  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_mutex_destroy(az_platform_mutex* mutex_handle)
{
  _az_PRECONDITION_NOT_NULL(mutex_handle);

  assert_ptr_equal(mutex_handle, test_mock_mutex);
  assert_int_equal(test_counter_mutex, 0);
  test_counter_mutex--;
  test_mock_mutex = NULL;

  return AZ_OK;
}
