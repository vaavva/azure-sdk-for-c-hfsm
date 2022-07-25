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

#include <azure/core/az_result.h>
#include <azure/core/az_hfsm.h>
#include <stdint.h>

/**
 * @brief USed to declare HFSM policies.
 * 
 */
// Definition is below.
typedef struct _az_hfsm_policy _az_hfsm_policy;

/**
 * @brief The type representing a HFSM with dispatch capabilities. Derived from #az_hfsm.
 *
 */
struct _az_hfsm_policy
{
  struct
  {
    /**
     * @brief Current state machine executing operations.
     * 
     */
    _az_hfsm_policy* inbound;
    _az_hfsm_policy* outbound;
    
    //HFSM_TODO: needs replacement with az_platform thread-safe Mailbox or Queue PAL.
    az_hfsm* hfsm;
    az_hfsm_event const* mailbox;
  } _internal;
};

/**
 * @brief Internal definition of an MQTT pipeline.
 *
 */
typedef struct
{
  struct
  {
    _az_hfsm_policy* last_policy;
  } _internal;
} az_hfsm_dispatch_pipeline;

// HFSM_TODO:
//      The idea is to have each client define its own pipeline. 
//      E.g. For the IoT client, the pipeline could be formed of the following HFSMs:
//        - IoT Orchestrator (DPS/Hub Retry System)
//        - IoT DPS (most of the times pass-through)
//        - IoT Hub
//        - MQTT Stack


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
AZ_NODISCARD az_result
az_hfsm_dispatch_post_inbound_event(_az_hfsm_policy* current,
    az_hfsm_event const* event);

AZ_NODISCARD az_result
az_hfsm_dispatch_post_outbound_event(_az_hfsm_policy* current,
    az_hfsm_event const* event);

#ifdef AZ_HFSM_PIPELINE_SYNC
/**
 * @brief Dispatches one event from the queue to a HFSM object using the current stack / thread.
 *
 * @note This function is to be used in conjunction with #az_hfsm_post_event. The HFSM is
 *       responsible with queue maintenance as well as destruction of the events after being
 *       consumed.
 *
 * @param[in] h The #az_hfsm to use for this call.
 */
AZ_NODISCARD az_result az_hfsm_dispatch_process_events(az_hfsm_dispatch_pipeline* h);
#endif

#endif //_az_HFSM_PIPELINE_H
