// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_event_policy.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_event_pipeline.h>
#include <azure/core/internal/az_hfsm.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result _az_event_pipeline_init(
    _az_event_pipeline* pipeline,
    az_event_policy* outbound,
    az_event_policy* inbound)
{
  pipeline->_internal.outbound_policy = outbound;
  pipeline->_internal.inbound_policy = inbound;
#ifndef TRANSPORT_MQTT_SYNC
  return az_platform_mutex_init(&pipeline->_internal.mutex);
#else
  return AZ_OK;
#endif
}

AZ_NODISCARD az_result
_az_event_pipeline_post_outbound_event(_az_event_pipeline* pipeline, az_event const event)
{
  az_result ret;

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));
#endif

  ret = _az_event_pipeline_send_outbound_event(pipeline->_internal.outbound_policy, event);

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));
#endif

  return ret;
}

AZ_NODISCARD az_result
_az_event_pipeline_post_inbound_event(_az_event_pipeline* pipeline, az_event const event)
{
  az_result ret;

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&pipeline->_internal.mutex));
#endif

  ret = _az_event_pipeline_send_inbound_event(pipeline->_internal.inbound_policy, event);

#ifndef TRANSPORT_MQTT_SYNC
  _az_RETURN_IF_FAILED(az_platform_mutex_release(&pipeline->_internal.mutex));
#endif

  return ret;
}

static void __az_event_pipeline_timer_callback(void* sdk_data)
{
  _az_event_pipeline_timer* timer = (_az_event_pipeline_timer*)sdk_data;
  az_event timer_event = (az_event){ .type = AZ_HFSM_EVENT_TIMEOUT, .data = timer };

  az_result ret = _az_event_pipeline_post_outbound_event(timer->_internal.pipeline, timer_event);

  if (az_result_failed(ret))
  {
    ret = _az_event_pipeline_post_inbound_event(
        timer->_internal.pipeline,
        (az_event){ .type = AZ_HFSM_EVENT_ERROR,
                    .data = &(az_hfsm_event_data_error){
                        .error_type = ret,
                        .sender_event = timer_event,
                        .sender = timer->_internal.pipeline->_internal.outbound_policy,
                    } });
  }

  if (az_result_failed(ret))
  {
    az_platform_critical_error();
  }
}

AZ_NODISCARD az_result
_az_event_pipeline_timer_create(_az_event_pipeline* pipeline, _az_event_pipeline_timer* out_timer)
{
  out_timer->_internal.pipeline = pipeline;

  return az_platform_timer_create(
      &out_timer->platform_timer, __az_event_pipeline_timer_callback, out_timer);
}

#ifdef TRANSPORT_MQTT_SYNC
// HFSM_TODO: Implement timer elapsed checks.

AZ_NODISCARD az_result _az_event_pipeline_sync_process_loop(az_event_pipeline* pipeline)
{
  // Process outbound events if any have been cached by the upper layers (e.g. API). This call is
  // blocking.
  _az_RETURN_IF_FAILED(_az_event_pipeline_post_outbound_event(
      pipeline, (az_event){ _az_event_pipeline_EVENT_PROCESS_LOOP, NULL }));

  // Process inbound events (e.g. network read). This call is blocking. When configurable, it should
  // wait at most AZ_MQTT_SYNC_MAX_POLLING_MILLISECONDS.
  _az_RETURN_IF_FAILED(_az_event_pipeline_post_inbound_event(
      pipeline, (az_event){ _az_event_pipeline_EVENT_PROCESS_LOOP, NULL }));

  // Process timer elapsed events.
  // HFSM_TODO: sync timer calculations.
}
#endif
