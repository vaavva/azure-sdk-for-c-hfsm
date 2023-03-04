/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Definition of #_az_event_pipeline and related types describing a bi-directional HFSM
 *        pipeline.
 *
 * @remarks Both a non-blocking I/O (default) as well as a blocking I/O implementations are
 * available. To enable the blocking mode, set TRANSPORT_MQTT_SYNC.
 *
 * @note The blocking I/O model (when TRANSPORT_MQTT_SYNC is defined) is not thread-safe.
 */

#ifndef _az_EVENT_PIPELINE_H
#define _az_EVENT_PIPELINE_H

#include <azure/core/az_event.h>
#include <azure/core/internal/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <stdint.h>

// Definition is below.
typedef struct _az_event_pipeline _az_event_pipeline;

/**
 * @brief Internal definition of an MQTT pipeline.
 *
 */
struct _az_event_pipeline
{
  struct
  {
    az_event_policy* outbound_policy;
    az_event_policy* inbound_policy;
#ifndef TRANSPORT_MQTT_SYNC
    az_platform_mutex mutex;
#endif
  } _internal;
};

AZ_NODISCARD az_result _az_event_pipeline_init(
    _az_event_pipeline* pipeline,
    az_event_policy* outbound,
    az_event_policy* inbound);

/**
 * @brief Queues an inbound event to the pipeline.
 *
 * @note The lifetime of the `event` must be maintained until the event is consumed by the
 *       HFSM. No threading guarantees exist for dispatching.
 *
 * @note This function should not be used during pipeline processing. The function can be called
 * either from the application or from within a system-level (timer, MQTT stack, etc) callback.
 *
 * @param[in] pipeline The #_az_event_pipeline to use for this call.
 * @param[in] event The event being enqueued.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
_az_event_pipeline_post_inbound_event(_az_event_pipeline* pipeline, az_event const event);

/**
 * @brief Queues an outbound event to the pipeline.
 *
 * @note The lifetime of the `event` must be maintained until the event is consumed by the
 *       HFSM. No threading guarantees exist for dispatching.
 *
 * @note This function should not be used during pipeline processing. The function can be called
 * either from the application or from within a system-level (timer, MQTT stack, etc) callback.
 *
 * @param[in] pipeline The #_az_event_pipeline to use for this call.
 * @param[in] event The event being enqueued.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
_az_event_pipeline_post_outbound_event(_az_event_pipeline* pipeline, az_event const event);

/**
 * @brief Sends an inbound event from the current policy.
 *
 * @note This function should be used during pipeline processing. The event is processed on the
 * current thread (stack).
 *
 * @param policy The current policy trying to send the event.
 * @param event The event being sent.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
_az_event_pipeline_send_inbound_event(az_event_policy* policy, az_event const event);

/**
 * @brief Sends an outbound event from the current policy.
 *
 * @note This function should be used during pipeline processing. The event is processed on the
 * current thread (stack).
 *
 * @param policy The current policy trying to send the event.
 * @param event The event being sent.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
_az_event_pipeline_send_outbound_event(az_event_policy* policy, az_event const event);

#ifdef TRANSPORT_MQTT_SYNC
enum az_hfsm_event_type_pipeline
{
  /**
   * @brief The event type posted by #_az_event_pipeline_sync_process_loop to allow syncrhonous
   * pipeline event processing.
   *
   */
  _az_event_pipeline_EVENT_PROCESS_LOOP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 9),
};

/**
 * @brief Blocking processing loop.
 * @note Application should call this only when TRANSPORT_MQTT_SYNC is defined.
 * @details This call will allow the MQTT stack to perform synchonous I/O. The current thread is
 * blocked until I/O is complete.
 *
 * @param pipeline The #_az_event_pipeline.
 * @return The #az_result error code.
 */
AZ_NODISCARD az_result _az_event_pipeline_sync_process_loop(_az_event_pipeline* pipeline);
#endif // TRANSPORT_MQTT_SYNC

/**
 * @brief Pipeline interval timer interface
 *
 */

typedef struct
{
  az_platform_timer platform_timer;

  struct
  {
    _az_event_pipeline* pipeline;
  } _internal;
} _az_event_pipeline_timer;

/**
 * @brief Creates an #az_platform_timer associated with an #_az_event_pipeline.
 * @details When the timer elapses, a TIMEOUT _outbound_ message will be generated. The event.#data
 *          contains a pointer to the original #az_platform_timer.
 * @param pipeline The pipeline.
 * @param[in, out] out_timer The populated timer structure.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
_az_event_pipeline_timer_create(_az_event_pipeline* pipeline, _az_event_pipeline_timer* out_timer);

#endif //_az_EVENT_PIPELINE_H
