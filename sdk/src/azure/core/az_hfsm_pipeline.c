// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_hfsm.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result
az_hfsm_pipeline_init(az_hfsm_pipeline* pipeline, az_hfsm_policy* outbound, az_hfsm_policy* inbound)
{
  pipeline->_internal.outbound_handler = outbound;
  pipeline->_internal.inbound_handler = inbound;
  return az_platform_mutex_init(&pipeline->_internal.mutex);
}

AZ_NODISCARD az_result
az_hfsm_pipeline_post_outbound_event(az_hfsm_pipeline* pipeline, az_hfsm_event event)
{
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));
  az_hfsm_send_event((az_hfsm*)pipeline->_internal.outbound_handler, event);
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));

  return AZ_OK;
}

AZ_NODISCARD az_result
az_hfsm_pipeline_post_inbound_event(az_hfsm_pipeline* pipeline, az_hfsm_event event)
{
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));
  az_hfsm_send_event((az_hfsm*)pipeline->_internal.inbound_handler, event);
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));

  return AZ_OK;
}

void az_hfsm_pipeline_error_handler(az_hfsm_policy* policy, az_result rc)
{
  if (az_result_failed(rc))
  {
    az_hfsm_event_data_error d = { .error_type = rc };
    az_result ret = az_hfsm_pipeline_post_inbound_event(
        policy->pipeline, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &d });

    if (az_result_failed(ret))
    {
      az_platform_critical_error();
    }
  }
}

// HFSM_TODO: Implement a sync version of the pipeline.
// HFSM_DESIGN: Add a base-class for az_hfsm_event_data containing the size. Event data will need to
//              either be pre-allocated by the application or copied into a queue/mailbox and
//              de-allocated after being dispatched.
