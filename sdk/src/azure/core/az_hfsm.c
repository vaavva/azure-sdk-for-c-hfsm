/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm.c
 * @brief Hierarchical Finite State Machine implementation.
 * 
 * @details This implementation is _not_ providing complete HFSM functionality. The following 
 *          constraints must be made by the developer for their state machines:
 *          1. A single top level state must exist.
 *          2. Transitions can only be made to sub-states, peer-states and super-states.
 *          3. The initial state is always the top-level state. Transitions must be made by the
 *             application if an inner state must be reached during initialization.
 */

#include <stdint.h>
#include <stddef.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/internal/az_precondition_internal.h>

const az_hfsm_event az_hfsm_entry_event = { AZ_HFSM_ENTRY, NULL };
const az_hfsm_event az_hfsm_exit_event = { AZ_HFSM_EXIT, NULL };

// Timer ID.
const az_hfsm_event az_hfsm_timeout_event = { AZ_HFSM_TIMEOUT, NULL };
const az_hfsm_event az_hfsm_error_unknown_event = { AZ_HFSM_ERROR, NULL };

/**
 * @brief Initializes the HFSM.
 * 
 * @param[in] h The HFSM handle.
 * @param root_state The root state for this HFSM.
 * @param get_parent_func The function describing the HFSM structure.
 * @return int32_t 
 */
int32_t az_hfsm_init(az_hfsm* h, az_hfsm_state_handler root_state, az_hfsm_get_parent get_parent_func)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(root_state);
  _az_PRECONDITION_NOT_NULL(get_parent_func);
  h->current_state = root_state;
  h->get_parent_func = get_parent_func;
  return h->current_state(h, az_hfsm_entry_event);
}

/**
 * @brief Exists all states up to, but excluding the source_state.
 * 
 * @details This is a partial implementation of finding the Least Common Ancestor (LCA) state, in 
 *          preparation for a state transition. A proper LCA is not implemeted as this HFSM is
 *          limited to 3 types of transitions: super, sub and peer.
 * 
 * @param h 
 * @param source_state 
 * @return int32_t 
 */
static int32_t _az_hfsm_recursive_exit(
    az_hfsm* h,
    az_hfsm_state_handler source_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);

  int32_t ret = 0;
  // Super-state handler making a transition must exit all substates:
  while (source_state != h->current_state)
  {
    // The current state cannot be null while walking the hierarchy chain from a sub-state to the
    // super-state: a top-level state is mandatory.
    _az_PRECONDITION_NOT_NULL(h->current_state);

    ret = h->current_state(h, az_hfsm_exit_event);
    az_hfsm_state_handler super_state = h->get_parent_func(h->current_state);
    _az_PRECONDITION_NOT_NULL(super_state);

    if (ret)
    {
      break;
    }

    h->current_state = super_state;
  }

  _az_PRECONDITION(h->current_state == source_state);

  return ret;
}

/**
 * @brief Transition to peer.
 * 
 * @details     Supported transitions limited to the following:
 *              - peer states (within the same top-level state).
 *              - super state transitioning to another peer state (all sub-states will exit).
 * @param[in] h The HFSM handle.
 * @param source_state The source state.
 * @param destination_state The destination state.
 * @return int32_t Non-zero return on error.
 */
int32_t az_hfsm_transition_peer(az_hfsm* h, az_hfsm_state_handler source_state, az_hfsm_state_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  int32_t ret = 0;
  // Super-state handler making a transition must exit all inner states:
  ret = _az_hfsm_recursive_exit(h, source_state);

  if (source_state == h->current_state)
  {
    // Exit the source state.
    ret = h->current_state(h, az_hfsm_exit_event);
    if (!ret)
    {
      // Enter the destination state:
      h->current_state = destination_state;
      ret = h->current_state(h, az_hfsm_entry_event);
    }
  }

  return ret;
}

/**
 * @brief Transition to sub-state.
 * 
 * @details Supported transitions limited to the following:
 *          - peer state transitioning to first-level sub-state.
 *          - super state transitioning to another first-level sub-state.
 * @param h The HFSM handle.
 * @param source_state The source state.
 * @param destination_state The destination state.
 * @return int32_t Non-zero return on error.
 */
int32_t az_hfsm_transition_substate(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(h->current_state);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  int32_t ret;
  // Super-state handler making a transition must exit all inner states:
  ret = _az_hfsm_recursive_exit(h, source_state);

  if (source_state == h->current_state)
  {
    // Transitions to sub-states will not exit the super-state:
    h->current_state = destination_state;
    ret = h->current_state(h, az_hfsm_entry_event);
  }

  return ret;
}


/**
 * @brief Transition to super-state
 *
 * @details Supported transitions limited to 
 *          - state transitioning to first-level super-state.
 *          - super-state transitioning to its immediate super-state.
 * @param h The HFSM handle.
 * @param source_state The source state.
 * @param destination_state The destination state.
 * @return int Non-zero return on error.
 */
int32_t hfsm_transition_superstate(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(h->current_state);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  int32_t ret;
  // Super-state handler making a transition must exit all inner states:
  ret = _az_hfsm_recursive_exit(h, source_state);

  if (source_state == h->current_state)
  {
    // Transitions to super states will exit the substate but not enter the superstate again:
    ret = h->current_state(h, az_hfsm_exit_event);
    h->current_state = destination_state;
  }

  return ret;
}

/**
 * @brief Post synchronous event.
 * 
 * @param h The HFSM handle.
 * @param event The posted event.
 * @return int Non-zero return on error.
 */
int32_t az_hfsm_send_event(az_hfsm* h, az_hfsm_event event)
{
  _az_PRECONDITION_NOT_NULL(h);
  int32_t ret;

  ret = h->current_state(h, event);

  az_hfsm_state_handler current = h->current_state;
  while (ret == AZ_HFSM_RET_HANDLE_BY_SUPERSTATE)
  {
    az_hfsm_state_handler super = h->get_parent_func(current);
    _az_PRECONDITION_NOT_NULL(super);
    current = super;
    ret = current(h, event);
  }

  return ret;
}
