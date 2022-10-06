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
  az_result ret;

  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));
  ret = az_hfsm_send_event((az_hfsm*)pipeline->_internal.outbound_handler, event);
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));

  return ret;
}

AZ_NODISCARD az_result
az_hfsm_pipeline_post_inbound_event(az_hfsm_pipeline* pipeline, az_hfsm_event event)
{
  az_result ret;

  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));
  ret = az_hfsm_send_event((az_hfsm*)pipeline->_internal.inbound_handler, event);
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));

  return ret;
}

void az_hfsm_pipeline_error_handler(az_hfsm_pipeline* pipeline, az_result rc)
{
  if (az_result_failed(rc))
  {
    az_hfsm_event_data_error d = { .error_type = rc };
    az_result ret
        = az_hfsm_pipeline_post_inbound_event(pipeline, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &d });

    if (az_result_failed(ret))
    {
      az_platform_critical_error();
    }
  }
}

void az_hfsm_pipeline_post_error(az_hfsm_pipeline* pipeline, az_result rc)
{
  if (az_result_failed(rc))
  {
    az_hfsm_event_data_error d = { .error_type = rc };
    az_result ret
        = az_hfsm_pipeline_post_inbound_event(pipeline, (az_hfsm_event){ AZ_HFSM_EVENT_ERROR, &d });

    if (az_result_failed(ret))
    {
      az_platform_critical_error();
    }
  }
}

static void _az_hfsm_pipeline_timer_callback(void* sdk_data)
{
  az_hfsm_pipeline_timer* timer = (az_hfsm_pipeline_timer*)sdk_data;
  az_result ret = az_hfsm_pipeline_post_outbound_event(
      timer->_internal.pipeline, (az_hfsm_event){ .type = AZ_HFSM_EVENT_TIMEOUT, .data = timer });

  az_hfsm_pipeline_error_handler(timer->_internal.pipeline, ret);
}

AZ_NODISCARD az_result
az_hfsm_pipeline_timer_create(az_hfsm_pipeline* pipeline, az_hfsm_pipeline_timer* out_timer)
{
  out_timer->_internal.pipeline = pipeline;

  return az_platform_timer_create(
      &out_timer->platform_timer, _az_hfsm_pipeline_timer_callback, out_timer);
}

// HFSM_TODO: Implement a sync version of the pipeline.
// HFSM_DESIGN: Add a base-class for az_hfsm_event_data containing the size. Event data will need to
//              either be pre-allocated by the application or copied into a queue/mailbox and
//              de-allocated after being dispatched.
