/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm_pipeline.h
 * @brief Definition of #az_hfsm_pipeline and related types describing a bi-directional HFSM
 *        pipeline.
 * 
 * @remarks Both a non-blocking I/O (default) as well as a blocking I/O implementations are
 * available. To enable the blocking mode, set TRANSPORT_MQTT_SYNC.
 * 
 * @note The blocking I/O model (when TRANSPORT_MQTT_SYNC is defined) is not thread-safe.
 */

#ifndef _az_HFSM_PIPELINE_H
#define _az_HFSM_PIPELINE_H

// Define AZ_HFSM_PIPELINE_SYNC to manually control when pipeline events are executed.
// The default action is to immediately process events on the same stack.
// #define AZ_HFSM_PIPELINE_SYNC

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <stdint.h>

/**
 * @brief USed to declare HFSM policies.
 *
 */
// Definition is below.
typedef struct az_hfsm_policy az_hfsm_policy;

typedef struct az_hfsm_pipeline az_hfsm_pipeline;

/**
 * @brief The type representing a HFSM with dispatch capabilities. Derived from #az_hfsm.
 *
 */
struct az_hfsm_policy
{
  az_hfsm hfsm; // Must be the first element to properly cast the struct to az_hfsm.
  az_hfsm_policy* inbound;
  az_hfsm_policy* outbound;
  az_hfsm_pipeline* pipeline;
};

/**
 * @brief Internal definition of an MQTT pipeline.
 *
 */
struct az_hfsm_pipeline
{
  struct
  {
    az_hfsm_policy* outbound_handler;
    az_hfsm_policy* inbound_handler;
#ifndef TRANSPORT_MQTT_SYNC
    az_platform_mutex mutex;
#endif
  } _internal;
};

AZ_NODISCARD az_result az_hfsm_pipeline_init(
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* outbound,
    az_hfsm_policy* inbound);

/**
 * @brief Queues an event to a HFSM object.
 *
 * @note The lifetime of the `event` must be maintained until the event is consumed by the
 *       HFSM. No threading guarantees exist for dispatching.
 *
 * @note This function should not be used during pipeline processing. The function can be called
 * either from the application or from within a system-level (timer, MQTT stack, etc) callback.
 *
 * @param[in] h The #az_hfsm_pipeline to use for this call.
 * @param[in] event The event being sent.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
az_hfsm_pipeline_post_inbound_event(az_hfsm_pipeline* pipeline, az_hfsm_event const event);

AZ_NODISCARD az_result
az_hfsm_pipeline_post_outbound_event(az_hfsm_pipeline* pipeline, az_hfsm_event const event);

/**
 * @brief Posts an inbound error event based on the #az_result `rc` parameter.
 * @details If `rc` is not an error code, no inbound message is posted.
 * @note This function should be used within system-level callbacks where a syncrhonous error cannot
 * be returned.
 *
 * @param pipeline The #az_hfsm_pipeline.
 * @param rc The #az_result error code.
 */
void az_hfsm_pipeline_post_error(az_hfsm_pipeline* pipeline, az_result rc);

// HFSM_TODO: policy_send_inbound / policy_send_outbound.
//            Handle errors by continuing the inbound operation in order to preserve the call-stack.

#ifdef TRANSPORT_MQTT_SYNC
enum az_hfsm_even_type_pipeline
{
  AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 9),
};

/**
 * @brief Blocking processing loop.
 * @note Application should call this only when TRANSPORT_MQTT_SYNC is defined.
 * @details This call will allow the MQTT stack to perform synchonous I/O. The current thread is
 * blocked until I/O is complete.
 *
 * @param pipeline The #az_hfsm_pipeline.
 * @return The #az_result error code.
 */
AZ_NODISCARD az_result az_hfsm_pipeline_syncrhonous_process_loop(az_hfsm_pipeline* pipeline);
#endif // TRANSPORT_MQTT_SYNC

typedef struct
{
  az_platform_timer platform_timer;

  struct
  {
    az_hfsm_pipeline* pipeline;
  } _internal;
} az_hfsm_pipeline_timer;

/**
 * @brief Creates an #az_platform_timer associated with an #az_hfsm_pipeline.
 * @details When the timer elapses, a TIMEOUT _outbound_ message will be generated. The event.#data
 *          contains a pointer to the original #az_platform_timer.
 * @param pipeline The pipeline.
 * @param[in, out] out_timer The populated timer structure.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
az_hfsm_pipeline_timer_create(az_hfsm_pipeline* pipeline, az_hfsm_pipeline_timer* out_timer);

#endif //_az_HFSM_PIPELINE_H
