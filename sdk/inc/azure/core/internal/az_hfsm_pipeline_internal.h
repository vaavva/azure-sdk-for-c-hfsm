/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * 
 * @brief Definition of #az_hfsm_pipeline and related types describing a bi-directional HFSM
 *        pipeline.
 *
 * @remarks Both a non-blocking I/O (default) as well as a blocking I/O implementations are
 * available. To enable the blocking mode, set PIPELINE_SYNC.
 *
 * @note The blocking I/O model (when PIPELINE_SYNC is defined) is not thread-safe and mutexes
 * are not used. Therefore, the application must ensure exclusion of all operations associated
 * with the pipeline.
 */

#ifndef _az_HFSM_PIPELINE_INTERNAL_H
#define _az_HFSM_PIPELINE_INTERNAL_H

#include <azure/core/internal/az_hfsm_internal.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct az_hfsm_policy az_hfsm_policy;

typedef struct az_hfsm_pipeline az_hfsm_pipeline;

/**
 * @brief The type representing an HFSM with dispatch capabilities. Derived from #az_hfsm.
 * Serves as a single element of an HFSM pipeline.
 */
struct az_hfsm_policy
{
  /// Must be the first element to properly cast the struct to az_hfsm.
  az_hfsm hfsm;

  az_hfsm_policy* inbound;

  az_hfsm_policy* outbound;
  
  az_hfsm_pipeline* pipeline;
};

/**
 * @brief The type representing an HFSM pipeline.
 *
 * @details Accepts external HFSM typed events which can be posted to the outbound
 * or inbound ends of the pipeline.
 */
struct az_hfsm_pipeline
{
  struct
  {
    az_hfsm_policy* outbound_handler;
    az_hfsm_policy* inbound_handler;
#ifndef PIPELINE_SYNC
    az_platform_mutex mutex;
#endif
  } _internal;
};

/**
 * @brief Initializes an HFSM pipeline.
 * 
 * @param[out] pipeline The #az_hfsm_pipeline to initialize.
 * @param[in] outbound The #az_hfsm_policy at the outbound end of the pipeline.
 * @param[in] inbound The #az_hfsm_policy at the inbound end of the pipeline.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result az_hfsm_pipeline_init(
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* outbound,
    az_hfsm_policy* inbound);

/**
 * @brief Queues an inbound event to the pipeline.
 *
 * @note The lifetime of the `event` must be maintained until the event is consumed by the
 *       HFSM. No threading guarantees exist for dispatching.
 *
 * @note This function should not be used during pipeline processing. The function can be called
 * either from the application or from within a system-level (timer, MQTT stack, etc) callback.
 *
 * @param[in] pipeline The #az_hfsm_pipeline to use for this call.
 * @param[in] event The event being enqueued.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result az_hfsm_pipeline_post_inbound_event(
    az_hfsm_pipeline* pipeline,
    az_hfsm_event const event);

/**
 * @brief Queues an outbound event to the pipeline.
 *
 * @note The lifetime of the `event` must be maintained until the event is consumed by the
 *       HFSM. No threading guarantees exist for dispatching.
 *
 * @note This function should not be used during pipeline processing. The function can be called
 * either from the application or from within a system-level (timer, MQTT stack, etc) callback.
 *
 * @param[in] pipeline The #az_hfsm_pipeline to use for this call.
 * @param[in] event The event being enqueued.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result az_hfsm_pipeline_post_outbound_event(
    az_hfsm_pipeline* pipeline,
    az_hfsm_event const event);

/**
 * @brief Sends an inbound event from the current policy.
 *
 * @note This function should be used during pipeline processing. The event is processed on the
 * current thread (stack).
 *
 * @param[in] policy The current policy trying to send the event.
 * @param[in] event The event being sent.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result az_hfsm_pipeline_send_inbound_event(
    az_hfsm_policy* policy,
    az_hfsm_event const event);

/**
 * @brief Sends an outbound event from the current policy.
 *
 * @note This function should be used during pipeline processing. The event is processed on the
 * current thread (stack).
 *
 * @param[in] policy The current policy trying to send the event.
 * @param[in] event The event being sent.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result az_hfsm_pipeline_send_outbound_event(
    az_hfsm_policy* policy,
    az_hfsm_event const event);

#ifdef PIPELINE_SYNC
enum az_hfsm_event_type_pipeline
{
  /**
   * @brief The event type posted by #az_hfsm_pipeline_sync_process_loop to allow synchronous
   * pipeline event processing.
   *
   */
  AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_MQTT, 9),
};

/**
 * @brief Blocking processing loop.
 * @note Application should call this only when PIPELINE_SYNC is defined.
 * @details This call will allow the MQTT stack to perform synchonous I/O. The current thread is
 * blocked until I/O is complete.
 *
 * @param[in] pipeline The #az_hfsm_pipeline.
 * 
 * @return The #az_result error code.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result az_hfsm_pipeline_sync_process_loop(az_hfsm_pipeline* pipeline);
#endif // PIPELINE_SYNC

/**
 * @brief Pipeline interval timer interface
 */
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
 * @details When the timer elapses, a TIMEOUT _outbound_ message will be generated. The `event.data`
 * contains a pointer to the original #az_platform_timer.
 * 
 * @param[in] pipeline The pipeline.
 * @param[in, out] out_timer The populated timer structure.
 * 
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
az_hfsm_pipeline_timer_create(az_hfsm_pipeline* pipeline, az_hfsm_pipeline_timer* out_timer);

#include <azure/core/_az_cfg_suffix.h>

#endif //_az_HFSM_PIPELINE_INTERNAL_H
