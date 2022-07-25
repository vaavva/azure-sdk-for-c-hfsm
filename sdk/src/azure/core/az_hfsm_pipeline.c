// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_hfsm.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_hfsm_dispatch_post_inbound_event(
    _az_hfsm_policy* current,
    az_hfsm_event const* event)
{
  _az_hfsm_policy* inbound_policy = current->_internal.inbound;

  if (inbound_policy == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

#ifndef AZ_HFSM_PIPELINE_SYNC
  // Async processes events immediately, on the same stack.
  az_hfsm_send_event(inbound_policy->_internal.hfsm, *event);
#else // AZ_HFSM_PIPELINE_SYNC

  // HFSM_TODO: Implement using queue 
  if (inbound_policy.mailbox != NULL)
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  inbound_policy.mailbox = event;
#endif //AZ_HFSM_PIPELINE_SYNC

  return AZ_OK;  
}

AZ_NODISCARD az_result az_hfsm_dispatch_post_outbound_event(
    _az_hfsm_policy* current,
    az_hfsm_event const* event)
{
  _az_hfsm_policy* outbound_policy = current->_internal.outbound;

  if (outbound_policy == NULL)
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

#ifndef AZ_HFSM_PIPELINE_SYNC
  // Async processes events immediately, on the same stack.
  az_hfsm_send_event(outbound_policy->_internal.hfsm, *event);
#else // AZ_HFSM_PIPELINE_SYNC

  // HFSM_TODO: Implement using queue 
  if (outbound_policy.mailbox != NULL)
  {
    return AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  outbound_policy.mailbox = event;
#endif //AZ_HFSM_PIPELINE_SYNC

  return AZ_OK;
}

#ifdef AZ_HFSM_PIPELINE_SYNC
void _az_hfsm_dispatch_process_current(_az_hfsm_policy* current)
{
  az_hfsm_event const* event = current->_internal.mailbox;

  if (event != NULL)
  {
    _az_PRECONDITION_NOT_NULL(current->_internal.hfsm);
    current->_internal.mailbox = NULL;
    az_hfsm_send_event(current->_internal.hfsm, event);
  }
}

void az_hfsm_dispatch_process_events(az_hfsm_dispatch_pipeline* pipeline)
{
  _az_hfsm_policy current = pipeline->_internal.last_policy;
  _az_PRECONDITION_NOT_NULL(current);

  do
  {
    _az_hfsm_dispatch_process_current(current);
    current = current->_internal.inbound;
  } while(current != NULL)

  return AZ_OK;
}
#endif
