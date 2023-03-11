/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm.h
 * @brief Definition of #az_hfsm and related types describing a Hierarchical Finite State Machine
 *        (HFSM).
 *
 *  @note All HFSM operations must be made thread-safe by the application. The code may execute on
 *        any thread as long as they are serialized.
 *
 *  @note All operations must be non-blocking.
 *
 *  @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_HFSM_H
#define _az_HFSM_H

#include <azure/core/az_result.h>
#include <azure/core/az_event_policy.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Common HFSM event types.
 *
 */
enum az_event_type_hfsm
{
  /// Entry event: must not set or use the data field, must be handled by each state.
  AZ_HFSM_EVENT_ENTRY = _az_MAKE_EVENT(_az_FACILITY_HFSM, 1),

  /// Exit event: must not set or use the data field, must be handled by each state.
  AZ_HFSM_EVENT_EXIT = _az_MAKE_EVENT(_az_FACILITY_HFSM, 2),
};

/**
 * @brief The type representing a Hierarchical Finite State Machine (HFSM) object.
 *
 */
typedef struct az_hfsm az_hfsm;

/**
 * @brief The generic state entry event.
 *
 * @note The entry and exit events must not expect a `data` field.
 */
extern const az_event az_hfsm_event_entry;

/**
 * @brief The generic state exit event.
 *
 * @note The entry and exit events must not expect a `data` field.
 */
extern const az_event az_hfsm_event_exit;

/**
 * @brief The type of the function returning the parent of any given child state.
 *
 * @note All HFSMs must define a `get_parent` function. A top-level state is mandatory.
 *       `get_parent(top_level_state)` must always return `NULL`.
 */
typedef az_event_policy_handler (*az_hfsm_get_parent)(az_event_policy_handler child_state);

// Avoiding a circular dependency between az_hfsm, az_event_policy_handler and az_hfsm_get_parent.
struct az_hfsm
{
  az_event_policy policy;

  struct
  {
    az_event_policy_handler current_state;
    az_hfsm_get_parent get_parent_func;
  } _internal;
};

/**
 * @brief Initializes a Hierarchical Finite State Machine (HFSM) object.
 *
 * @param[out] h The #az_hfsm to use for this call.
 * @param[in] root_state The top-level (root) state of this HFSM.
 * @param[in] get_parent_func A pointer to the implementation of the #az_hfsm_get_parent function
 *            defining this HFSM's hierarchy.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result
az_hfsm_init(az_hfsm* h, az_event_policy_handler root_state, az_hfsm_get_parent get_parent_func);

/**
 * @brief Transition to a peer state.
 *
 * @note  Calling this as a result of AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE is supported.
 *        The application developer is responsible with ensuring the hierarchy is properly defined
 *        and no invalid calls are made.
 *
 * @param[in] h The #az_hfsm to use for this call.
 * @param[in] source_state The source state.
 * @param[in] destination_state The destination state.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_hfsm_transition_peer(
    az_hfsm* h,
    az_event_policy_handler source_state,
    az_event_policy_handler destination_state);

/**
 * @brief Transition to a sub state.
 *
 * @note  Calling this as a result of AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE is supported.
 *        The application developer is responsible with ensuring the hierarchy is properly defined
 *        and no invalid calls are made.
 *
 * @param[in] h The #az_hfsm to use for this call.
 * @param[in] source_state The source state.
 * @param[in] destination_state The destination state.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_hfsm_transition_substate(
    az_hfsm* h,
    az_event_policy_handler source_state,
    az_event_policy_handler destination_state);

/**
 * @brief Transition to a super state.
 *
 * @note  Calling this as a result of AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE is supported.
 *        The application developer is responsible with ensuring the hierarchy is properly defined
 *        and no invalid calls are made.
 *
 * @param[in] h The #az_hfsm to use for this call.
 * @param[in] source_state The source state.
 * @param[in] destination_state The destination state.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_hfsm_transition_superstate(
    az_hfsm* h,
    az_event_policy_handler source_state,
    az_event_policy_handler destination_state);

/**
 * @brief Synchronously sends an event to a HFSM object.
 *
 * @note The lifetime of the event's `data` argument must be guaranteed until this function returns.
 *       All state handlers related to this event will execute on the current stack. In most cases
 *       it is recommended that a queue together with a message pump is used as an intermediary to
 *       avoid recursive calls.
 *
 * @param[in] h The #az_hfsm to use for this call.
 * @param[in] event The event being sent.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_hfsm_send_event(az_hfsm* h, az_event event);

#include <azure/core/_az_cfg_suffix.h>

#endif //_az_HFSM_H
