/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm.c
 * @brief Hierarchical Finite State Machine (HFSM) implementation.
 *
 * @details This implementation is _not_ providing complete HFSM functionality. The following
 *          constraints must be made by the developer for their state machines:
 *          1. A single top level state must exist.
 *          2. Transitions can only be made to sub-states, peer-states and super-states.
 *          3. The initial state is always the top-level state. Transitions must be made by the
 *             application if an inner state must be reached during initialization.
 */

#include <stddef.h>
#include <stdint.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/_az_cfg.h>

const az_hfsm_event az_hfsm_event_entry = { AZ_HFSM_EVENT_ENTRY, NULL };
const az_hfsm_event az_hfsm_event_exit = { AZ_HFSM_EVENT_EXIT, NULL };

AZ_NODISCARD az_result
az_hfsm_init(az_hfsm* h, az_hfsm_state_handler root_state, az_hfsm_get_parent get_parent_func)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(root_state);
  _az_PRECONDITION_NOT_NULL(get_parent_func);
  h->_internal.current_state = root_state;
  h->_internal.get_parent_func = get_parent_func;

  // Root state entry must always return a "handled" status.
  _az_PRECONDITION(AZ_HFSM_RETURN_HANDLED == h->_internal.current_state(h, az_hfsm_event_entry));

  return AZ_OK;
}

static void _az_hfsm_recursive_exit(az_hfsm* h, az_hfsm_state_handler source_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);

  // Super-state handler making a transition must exit all substates:
  while (source_state != h->_internal.current_state)
  {
    // A top-level state is mandatory to ensure an LCA exists.
    _az_PRECONDITION_NOT_NULL(h->_internal.current_state);

    _az_PRECONDITION(AZ_HFSM_RETURN_HANDLED == h->_internal.current_state(h, az_hfsm_event_exit));
    az_hfsm_state_handler super_state = h->_internal.get_parent_func(h->_internal.current_state);
    _az_PRECONDITION_NOT_NULL(super_state);

    h->_internal.current_state = super_state;
  }
}

void az_hfsm_transition_peer(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  // Super-state handler making a transition must exit all inner states:
  _az_hfsm_recursive_exit(h, source_state);
  _az_PRECONDITION(h->_internal.current_state == source_state);

  // Exit the source state.
  _az_PRECONDITION(AZ_HFSM_RETURN_HANDLED == h->_internal.current_state(h, az_hfsm_event_exit));

  // Enter the destination state:
  h->_internal.current_state = destination_state;
  _az_PRECONDITION(AZ_HFSM_RETURN_HANDLED == h->_internal.current_state(h, az_hfsm_event_entry));
}

void az_hfsm_transition_substate(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  // Super-state handler making a transition must exit all inner states:
  _az_hfsm_recursive_exit(h, source_state);
  _az_PRECONDITION(h->_internal.current_state == source_state);

  // Transitions to sub-states will not exit the super-state:
  h->_internal.current_state = destination_state;
  _az_PRECONDITION(AZ_HFSM_RETURN_HANDLED == h->_internal.current_state(h, az_hfsm_event_entry));
}

void hfsm_transition_superstate(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state)
{
  _az_PRECONDITION_NOT_NULL(h);
  _az_PRECONDITION_NOT_NULL(source_state);
  _az_PRECONDITION_NOT_NULL(destination_state);

  // Super-state handler making a transition must exit all inner states:
  _az_hfsm_recursive_exit(h, source_state);
  _az_PRECONDITION(h->_internal.current_state == source_state);

  // Transitions to super states will exit the substate but not enter the superstate again:
  _az_PRECONDITION(AZ_HFSM_RETURN_HANDLED == h->_internal.current_state(h, az_hfsm_event_exit));
  h->_internal.current_state = destination_state;
}

void az_hfsm_send_event(az_hfsm* h, az_hfsm_event event)
{
  _az_PRECONDITION_NOT_NULL(h);
  az_hfsm_return_type ret;

  az_hfsm_state_handler current = h->_internal.current_state;
  _az_PRECONDITION_NOT_NULL(current);
  ret = current(h, event);

  while (ret == AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE)
  {
    az_hfsm_state_handler super = h->_internal.get_parent_func(current);

    // Top-level state must handle _all_ events.
    _az_PRECONDITION_NOT_NULL(super);
    current = super;
    ret = current(h, event);
  }

  _az_PRECONDITION(ret == AZ_HFSM_RETURN_HANDLED);
}
