// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_hfsm.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg.h>

void az_hfsm_pipeline_post_event(az_hfsm_pipeline* pipeline, az_hfsm_event event)
{
  _az_PRECONDITION_NOT_NULL(pipeline->_internal.first_policy);

  az_hfsm_send_event(pipeline->_internal.first_policy->hfsm, event);
}

void az_hfsm_pipeline_post_inbound_event(az_hfsm_policy* current, az_hfsm_event event)
{
  az_hfsm_policy* inbound_policy = current->inbound;
  _az_PRECONDITION_NOT_NULL(inbound_policy);

  // Async processes events immediately, on the same stack.
  az_hfsm_send_event(inbound_policy->hfsm, event);
}

void az_hfsm_pipeline_post_outbound_event(az_hfsm_policy* current, az_hfsm_event event)
{
  az_hfsm_policy* outbound_policy = current->outbound;
  _az_PRECONDITION_NOT_NULL(outbound_policy);

  // Async processes events immediately, on the same stack.
  az_hfsm_send_event(outbound_policy->hfsm, event);
}

// HFSM_TODO: Implement a sync version of the pipeline.
// HFSM_DESIGN: Add a base-class for az_hfsm_event_data containing the size. Event data will need to 
//              either be pre-allocated by the application or copied into a queue/mailbox and
//              de-allocated after being dispatched. 
