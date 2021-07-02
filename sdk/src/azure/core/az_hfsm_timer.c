/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm_timer.c
 * @brief Hierarchical Finite State Machine (HFSM) timer adapter.
 *
 */

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

static void _az_hfsm_timer_callback(void* sdk_data)
{
  _az_PRECONDITION_NOT_NULL(sdk_data);

  // sdk_data must always be of type #az_hfsm_timer_sdk_data.
  az_hfsm_timer_sdk_data* data = (az_hfsm_timer_sdk_data*)sdk_data;

  _az_PRECONDITION_NOT_NULL(data->_internal.target_hfsm);
  _az_PRECONDITION_NOT_NULL(data->_internal.event);

  // The event is processed on the timer thread.
  az_hfsm_send_event(data->_internal.target_hfsm, *data->_internal.event);
}

AZ_NODISCARD az_result az_hfsm_timer_create(
    az_hfsm* hfsm,
    az_hfsm_timer_sdk_data* timer_data,
    void** out_timer_handle)
{
  timer_data->_internal.target_hfsm = hfsm;
  return az_platform_timer_create(_az_hfsm_timer_callback, timer_data, out_timer_handle);
}
