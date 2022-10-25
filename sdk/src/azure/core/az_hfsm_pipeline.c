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
#ifndef TRANSPORT_MQTT_SYNC
  return az_platform_mutex_init(&pipeline->_internal.mutex);
#else
  return AZ_OK;
#endif
}

AZ_NODISCARD az_result
az_hfsm_pipeline_post_outbound_event(az_hfsm_pipeline* pipeline, az_hfsm_event event)
{
  az_result ret;

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));
#endif

  ret = az_hfsm_send_event((az_hfsm*)pipeline->_internal.outbound_handler, event);

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));
#endif

  return ret;
}

AZ_NODISCARD az_result
az_hfsm_pipeline_post_inbound_event(az_hfsm_pipeline* pipeline, az_hfsm_event event)
{
  az_result ret;

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));
#endif

  ret = az_hfsm_send_event((az_hfsm*)pipeline->_internal.inbound_handler, event);

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));
#endif

  return ret;
}

AZ_NODISCARD az_result
az_hfsm_pipeline_send_indbound_event(az_hfsm_policy* policy, az_hfsm_event const event)
{
  az_result ret = az_hfsm_send_event((az_hfsm*)policy->inbound, event);
  if (az_result_failed(ret))
  {
    // Replace the original event with an error event that is flowed to the application.
    ret = az_hfsm_send_event(
        (az_hfsm*)policy->inbound,
        (az_hfsm_event){ AZ_HFSM_EVENT_ERROR,
                         &(az_hfsm_event_data_error){
                             .error_type = ret,
                             .sender_hfsm = (az_hfsm*)policy->inbound,
                             .sender_event = event,
                         } });
  }

  return ret;
}

AZ_NODISCARD az_result
az_hfsm_pipeline_send_outbound_event(az_hfsm_policy* policy, az_hfsm_event const event)
{
  // The error is flowed back to the application.
  return az_hfsm_send_event((az_hfsm*)policy->outbound, event);
}

static void _az_hfsm_pipeline_timer_callback(void* sdk_data)
{
  az_hfsm_pipeline_timer* timer = (az_hfsm_pipeline_timer*)sdk_data;
  az_hfsm_event timer_event = (az_hfsm_event){ .type = AZ_HFSM_EVENT_TIMEOUT, .data = timer };

  az_result ret = az_hfsm_pipeline_post_outbound_event(timer->_internal.pipeline, timer_event);

  if (az_result_failed(ret))
  {
    ret = az_hfsm_pipeline_post_inbound_event(
        timer->_internal.pipeline,
        (az_hfsm_event){
            .type = AZ_HFSM_EVENT_ERROR,
            .data = &(az_hfsm_event_data_error){
                .error_type = ret,
                .sender_event = timer_event,
                .sender_hfsm = (az_hfsm*)timer->_internal.pipeline->_internal.outbound_handler,
            } });
  }

  if (az_result_failed(ret))
  {
    az_platform_critical_error();
  }
}

AZ_NODISCARD az_result
az_hfsm_pipeline_timer_create(az_hfsm_pipeline* pipeline, az_hfsm_pipeline_timer* out_timer)
{
  out_timer->_internal.pipeline = pipeline;

  return az_platform_timer_create(
      &out_timer->platform_timer, _az_hfsm_pipeline_timer_callback, out_timer);
}

#ifdef TRANSPORT_MQTT_SYNC
AZ_NODISCARD az_result az_hfsm_pipeline_sync_process_loop(az_hfsm_pipeline* pipeline)
{
  // Process outbound events if any have been cached by the upper layers (e.g. API). This call is
  // blocking.
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_post_outbound_event(
      pipeline, (az_hfsm_event){ AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP, NULL }));

  // Process inbound events (e.g. network read). This call is blocking. When configurable, it should
  // wait at most AZ_MQTT_SYNC_MAX_POLLING_MILLISECONDS.
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_post_inbound_event(
      pipeline, (az_hfsm_event){ AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP, NULL }));
}
#endif
