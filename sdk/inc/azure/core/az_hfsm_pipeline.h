/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm_pipeline.h
 * @brief Definition of #az_hfsm_pipeline and related types describing a bi-directional HFSM
 *        pipeline.
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
    az_platform_mutex mutex;
  } _internal;
};

AZ_NODISCARD az_result az_hfsm_pipeline_init(
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* outbound,
    az_hfsm_policy* inbound);

/**
 * @brief Queues an event to a HFSM object.
 *
 * @note The lifetime of the `event_reference` must be maintained until the event is consumed by the
 *       HFSM. No threading guarantees exist for dispatching.
 *
 * @param[in] h The #az_hfsm to use for this call.
 * @param[in] event_reference A reference to the event being sent.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_hfsm_pipeline_post_inbound_event(
    az_hfsm_pipeline* pipeline,
    az_hfsm_event const event);

AZ_NODISCARD az_result az_hfsm_pipeline_post_outbound_event(
    az_hfsm_pipeline* pipeline,
    az_hfsm_event const event);

void az_hfsm_pipeline_error_handler(az_hfsm_policy* policy, az_result rc);

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
AZ_NODISCARD az_result az_hfsm_pipeline_timer_create(
  az_hfsm_pipeline* pipeline,
  az_hfsm_pipeline_timer* out_timer);

#endif //_az_HFSM_PIPELINE_H
