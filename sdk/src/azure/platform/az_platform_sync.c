// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_platform_sync.c
 * @brief Defines a very simple syncrhonous platform adapter.
 * 
 * @note This sync adapter is limited to exactly one timer operation and single element queues.
 * 
 */

#include <azure/core/az_result.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg.h>

typedef int32_t timer_handle_type;

typedef struct
{
    // A single timer is supported within this implementation..
    timer_handle_type timer_handle;
    az_platform_timer_callback callback;
    void* sdk_data;
} single_timer_type;

single_timer_type single_timer;

AZ_NODISCARD az_result az_platform_timer_create(
    az_platform_timer_callback callback,
    void* sdk_data,
    void** out_timer_handle)
{
    az_result ret;

    if (single_timer.timer_handle == 0)
    {
        single_timer.timer_handle = 1;
        single_timer.callback = callback;
        single_timer.sdk_data = sdk_data;
        *out_timer_handle = &single_timer.timer_handle;

        ret = AZ_OK;
    }
    else
    {
        // A single timer is supported for time-delay operations.
        ret = AZ_ERROR_NOT_SUPPORTED;
    }

    return ret;
}

AZ_NODISCARD az_result az_platform_timer_start(void* timer_handle, int32_t milliseconds)
{
    az_result ret;
    _az_PRECONDITION_NOT_NULL(timer_handle);
    _az_PRECONDITION(timer_handle == &single_timer.timer_handle);
    
    if (timer_handle == &single_timer.timer_handle)
    {
        _az_PRECONDITION(az_result_succeeded(az_platform_sleep_msec(milliseconds)));
        single_timer.callback(single_timer.sdk_data);
        ret = AZ_OK;
    }
    else
    {
        ret = AZ_ERROR_NOT_SUPPORTED;
    }

    return ret;
}

void az_platform_timer_destroy(void* timer_handle)
{
   _az_PRECONDITION_NOT_NULL(timer_handle);
   _az_PRECONDITION(timer_handle == &single_timer.timer_handle);

   single_timer.timer_handle = 0;
}

// The queue type for sync is a single void const* element.
typedef void const* single_element_queue;

AZ_NODISCARD az_result az_platform_queue_push(void* queue_handle, void const* element)
{
  // Simulate a single-element queue. queue_handle must be defined and initialized as a void const* single element.
  _az_PRECONDITION_NOT_NULL(queue_handle);
  _az_PRECONDITION_NOT_NULL(element);
  az_result ret;
  
  single_element_queue* q = (single_element_queue*)queue_handle;

  if (*q == NULL)
  {
      *q = element;
      ret = AZ_OK;
  }
  else
  {
      ret = AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  return ret;
}

AZ_NODISCARD az_result az_platform_queue_pop(void* queue_handle, void const** out_element)
{
  _az_PRECONDITION_NOT_NULL(queue_handle);
  _az_PRECONDITION_NOT_NULL(out_element);
  az_result ret;
     
  single_element_queue* q = (single_element_queue*)queue_handle;

  if (*q == NULL)
  {
      ret = AZ_ERROR_ITEM_NOT_FOUND;
  }
  else
  {
      *out_element = *q;
      ret = AZ_OK;
  }

  return ret;
}
