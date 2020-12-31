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

#include <azure/core/az_result.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief HSM event type.
 *
 */
typedef enum
{
  AZ_IOT_HSM_ENTRY = 1,
  AZ_IOT_HSM_EXIT = 2,
  AZ_IOT_HSM_ERROR = 3,
  AZ_IOT_HSM_TIMEOUT = 4,
  _az_IOT_HSM_EVENT_BASE = 10,
} az_iot_hsm_event_type;

#define _az_IOT_HSM_EVENT(id) ((int32_t)(_az_IOT_HSM_EVENT_BASE + id))

typedef struct az_iot_hsm_event az_iot_hsm_event;
typedef struct az_iot_hsm az_iot_hsm;
typedef az_result (*state_handler)(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)());

struct az_iot_hsm_event
{
  az_iot_hsm_event_type type;
  void* data;
};

struct az_iot_hsm
{
  state_handler current_state;
};

extern const az_iot_hsm_event az_iot_hsm_entry_event;
extern const az_iot_hsm_event az_iot_hsm_exit_event;
extern const az_iot_hsm_event az_iot_hsm_timeout_event;

AZ_INLINE az_result az_iot_hsm_init(az_iot_hsm* h, state_handler initial_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(initial_state);
  // TODO - avoiding C28182 - this should be handled by the precondition.
  if (initial_state == NULL)
  {
    return AZ_ERROR_ARG;
  }

  h->current_state = initial_state;
  return h->current_state(h, az_iot_hsm_entry_event, NULL);
}

AZ_INLINE az_result _az_iot_hsm_recursive_exit(
    az_iot_hsm* h,
    state_handler source_state,
    state_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  az_result ret = AZ_OK;
  // Super-state handler making a transition must exit all inner states:
  while (source_state != h->current_state)
  {
    // The current state cannot be null while walking the hierarchy chain from an inner state to the
    // super-state:
    _az_PRECONDITION_NOT_NULL(h->current_state);

    state_handler super_state;
    ret = h->current_state(h, az_iot_hsm_exit_event, &super_state);
    if (az_result_failed(ret))
    {
      break;
    }

    h->current_state = super_state;
  }

  return ret;
}

/*
    Supported transitions limited to the following:
    - peer states (within the same top-level state).
    - super state transitioning to another peer state (all inner states will exit).
*/
AZ_INLINE az_result
az_iot_hsm_transition(az_iot_hsm* h, state_handler source_state, state_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  az_result ret = AZ_OK;
  // Super-state handler making a transition must exit all inner states:
  ret = _az_iot_hsm_recursive_exit(h, source_state, destination_state);

  if (source_state == h->current_state)
  {
    // Exit the source state.
    ret = h->current_state(h, az_iot_hsm_exit_event, NULL);
    if (!az_result_failed(ret))
    {
      // Enter the destination state:
      h->current_state = destination_state;
      ret = h->current_state(h, az_iot_hsm_entry_event, NULL);
    }
  }

  return ret;
}

/*
    Supported transitions limited to the following:
    - peer state transitioning to first-level inner state.
    - super state transitioning to another first-level inner state.
*/
AZ_INLINE az_result az_iot_hsm_transition_inner(
    az_iot_hsm* h,
    state_handler source_state,
    state_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(h->current_state);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  // TODO - avoiding C28182 - this should be handled by the precondition.
  if (destination_state == NULL)
  {
    return AZ_ERROR_ARG;
  }

  az_result ret;
  // Super-state handler making a transition must exit all inner states:
  ret = _az_iot_hsm_recursive_exit(h, source_state, destination_state);

  if (source_state == h->current_state)
  {
    // Transitions to inner states will not exit the super-state:
    h->current_state = destination_state;
    ret = h->current_state(h, az_iot_hsm_entry_event, NULL);
  }

  return ret;
}

AZ_INLINE bool az_iot_hsm_post_sync(az_iot_hsm* h, az_iot_hsm_event event)
{
  _az_PRECONDITION_NOT_NULL(h);
  return h->current_state(h, event, NULL);
}

#include <azure/core/_az_cfg_suffix.h>

#endif //!_az_IOT_HSM_INTERNAL_H
