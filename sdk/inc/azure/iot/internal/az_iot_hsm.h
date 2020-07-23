// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_hsm.h
 *
 * @brief Azure IoT Hierarchical State Machine.
 * https://en.wikipedia.org/wiki/UML_state_machine#Hierarchically_nested_states
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_IOT_HSM_INTERNAL_H
#define _az_IOT_HSM_INTERNAL_H

#include <stdint.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Twin response type.
 *
 */
typedef enum
{
  AZ_IOT_HSM_ENTRY = 1,
  AZ_IOT_HSM_EXIT = 2,
  AZ_IOT_HSM_ERROR = 3,
  AZ_IOT_HSM_TIMEOUT = 4,
} az_iot_hsm_event_type;

typedef struct az_iot_hsm_event
{
    az_iot_hsm_event_type type;
    void* data;
} az_iot_hsm_event;

const az_iot_hsm_event az_iot_hsm_entry_event = { AZ_IOT_HSM_ENTRY, NULL };
const az_iot_hsm_event az_iot_hsm_exit_event = { AZ_IOT_HSM_EXIT, NULL };

typedef struct az_iot_hsm
{   
  struct az_iot_hsm * super;
  az_result (*current_state)(struct az_iot_hsm *me, az_iot_hsm_event event);
} az_iot_hsm;

AZ_INLINE az_result az_iot_hsm_init(az_iot_hsm* h, az_iot_hsm* super, az_result (*initial_state)(az_iot_hsm *me, az_iot_hsm_event event))
{
    _az_PRECONDITION_NOT_NULL(h);
    _az_PRECONDITION_NOT_NULL(initial_state);
    // TODO - avoiding C28182 - this should be handled by the precondition.
    if (initial_state == NULL) return AZ_ERROR_ARG;

    h->super = super; // if NULL, this is the top-level state machine.
    h->current_state = initial_state;
    return h->current_state(h, az_iot_hsm_entry_event);
}

AZ_INLINE az_result az_iot_hsm_transition(az_iot_hsm* h, az_result (*next_state)(az_iot_hsm *me, az_iot_hsm_event event))
{
    _az_PRECONDITION_NOT_NULL(h);
    _az_PRECONDITION_NOT_NULL(h->current_state);
    _az_PRECONDITION_NOT_NULL(next_state);
    // TODO - avoiding C28182 - this should be handled by the precondition.
    if (next_state == NULL) return AZ_ERROR_ARG;

    az_result ret;
    ret = h->current_state(h, az_iot_hsm_exit_event);
    if (!az_failed(ret))
    {
        h->current_state = next_state;
        ret = h->current_state(h, az_iot_hsm_entry_event);
    }

    return ret;
}

AZ_INLINE az_result az_iot_hsm_super(az_iot_hsm* h, az_iot_hsm_event event)
{
    _az_PRECONDITION_NOT_NULL(h);
    _az_PRECONDITION_NOT_NULL(h->super);

    return h->super->current_state(h->super, event);
}

AZ_INLINE bool az_iot_hsm_post_sync(az_iot_hsm* h, az_iot_hsm_event event)
{
    _az_PRECONDITION_NOT_NULL(h);
    return h->current_state(h, event);
}

#include <azure/core/_az_cfg_suffix.h>

#endif //!_az_IOT_HSM_INTERNAL_H
