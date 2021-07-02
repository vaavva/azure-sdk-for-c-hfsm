/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm_dispatch.h
 * @brief Definition of #az_hfsm_dispatch, a HFSM with message queuing support.
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_HFSM_DISPATCH_H
#define _az_HFSM_DISPATCH_H

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h


/**
 * @brief The type representing a HFSM with dispatch capabilities. Derived from #az_hfsm.
 * 
 */
typedef struct
{
    struct 
    {
        az_hfsm hfsm;
        void* queue_handle;
    } _internal;
} az_hfsm_dispatch;

/**
 * @brief Adds dispatch capabilities to an existing HFSM.
 *
 * @param[out] h The #az_hfsm to use for this call.
 * @param[in] hfsm The HFSM.
 * @param[in] queue_handle The handle to the queue object.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_hfsm_dispatch_init(
    az_hfsm_dispatch* h,
    az_hfsm const* hfsm
    void const* queue_handle);

/**
 * @brief Queues an event to a HFSM object.
 * 
 * @note The lifetime of the event's `data` must be maintained until the event is consumed by the
 *       HFSM. The state handlers related to this event may execute on other threads.
 *       This call may send AZ_HFSM_ERROR on queue errors.
 * 
 * @param[in] h The #az_hfsm to use for this call.
 * @param[in] event The event being sent.
 */
void az_hfsm_dispatch_post_event(az_hfsm_dispatch* h, az_hfsm_event event);

/**
 * @brief Dispatches one event from the queue to a HFSM object using the current stack / thread.
 * 
 * @note This function is to be used in conjunction with #az_hfsm_post_event. The HFSM is 
 *       responsible with queue maintenance as well as destruction of the events after being 
 *       consumed.
 * 
 * @param[in] h The #az_hfsm to use for this call.
 */
AZ_NODISCARD az_result az_hfsm_dispatch_one(az_hfsm_dispatch* h);
#endif //_az_HFSM_DISPATCH_H
