/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm_dispatch.c
 * @brief Hierarchical Finite State Machine (HFSM) dispatcher adapter.
 *
 */

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

AZ_NODISCARD az_result
az_hfsm_dispatch_post_event(az_hfsm_dispatch* h, az_hfsm_event const* event)
{
  return az_platform_queue_push(h->_internal.queue_handle, event);
}

AZ_NODISCARD az_result az_hfsm_dispatch_one(az_hfsm_dispatch* h)
{
  az_hfsm_event* event;
  _az_RETURN_IF_FAILED(az_platform_queue_pop(h->_internal.queue_handle, &event));
  az_hfsm_send_event(h->_internal.hfsm, *event);

  return AZ_OK;
}
