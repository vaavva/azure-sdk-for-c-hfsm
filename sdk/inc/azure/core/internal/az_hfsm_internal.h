/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file 
 * 
 * @brief Definition of #_az_hfsm and related types describing a Hierarchical Finite State Machine
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

#ifndef _az_HFSM_INTERNAL_H
#define _az_HFSM_INTERNAL_H

#include <azure/core/az_result.h>
#include <azure/core/az_event.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

/**
 * @brief Common HFSM event types.
 *
 */
enum _az_event_type_hfsm
{

  /**
   * @brief Entry event: must not set or use the data field, must be handled by each state.
   */ 
  AZ_HFSM_EVENT_ENTRY = _az_MAKE_EVENT(_az_FACILITY_HFSM, 1),
 
  /**
   * @brief Exit event: must not set or use the data field, must be handled by each state.
   */
  AZ_HFSM_EVENT_EXIT = _az_MAKE_EVENT(_az_FACILITY_HFSM, 2),

  /**
   * @brief Generic error event: must use a data field containing a structure derived from
   *        #az_hfsm_error_data
  */
  AZ_HFSM_EVENT_ERROR = _az_MAKE_EVENT(_az_FACILITY_HFSM, 3),

  /**
   * @brief Generic timeout event: if multiple timers are necessary it's recommended to create
   *        separate timeout events.
   */
  AZ_HFSM_EVENT_TIMEOUT = _az_MAKE_EVENT(_az_FACILITY_HFSM, 4),
};

/**
 * @brief HFSM specific az_result types.
 */
enum _az_result_hfsm
{
  /**
   * @brief Indicates to the HFSM engine that the superstate should handle the event.
   * 
   * @details This event should not be leaked out of the HFSM system (i.e. the root state should
   *          never return this az_result type). The event indicates normal operation but was
   *          constructed as an error to detect this case.
   */
  AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE = _az_RESULT_MAKE_ERROR(_az_FACILITY_HFSM, 0),
};

/**
 * @brief The type representing a Hierarchical Finite State Machine (HFSM) object.
 */
typedef struct _az_hfsm _az_hfsm;

/**
 * @brief The type representing the minimum data required for an #AZ_HFSM_EVENT_ERROR event.
 */
typedef struct
{
  /**
   * @brief The error type as an #az_result.
   */
  az_result error_type;

  void* sender;
  
  az_event sender_event;
} _az_hfsm_event_data_error;

/**
 * @brief The generic state entry event.
 *
 * @note The entry and exit events must not expect a `data` field.
 */
extern const az_event _az_hfsm_event_entry;

/**
 * @brief The generic state exit event.
 *
 * @note The entry and exit events must not expect a `data` field.
 */
extern const az_event _az_hfsm_event_exit;

/**
 * @brief The function signature for all HFSM states.
 *
 * @note All HFSM states must follow the pattern of a single `switch ((int32_t)event.type)`
 *       statement. Code should not exist outside of the switch statement.
 */
typedef az_result (*_az_hfsm_state_handler)(_az_hfsm* me, az_event event);

/**
 * @brief The type of the function returning the parent of any given child state.
 *
 * @note All HFSMs must define a `get_parent` function. A top-level state is mandatory.
 *       `get_parent(top_level_state)` must always return `NULL`.
 */
typedef _az_hfsm_state_handler (*_az_hfsm_get_parent)(_az_hfsm_state_handler child_state);

// Avoiding a circular dependency between _az_hfsm, _az_hfsm_state_handler and _az_hfsm_get_parent.

struct _az_hfsm
{
  struct
  {
    _az_hfsm_state_handler current_state;
    _az_hfsm_get_parent get_parent_func;
  } _internal;
};

/**
 * @brief Initializes a Hierarchical Finite State Machine (HFSM) object.
 *
 * @param[out] h The #_az_hfsm to use for this call.
 * @param[in] root_state The top-level (root) state of this HFSM.
 * @param[in] get_parent_func A pointer to the implementation of the #_az_hfsm_get_parent function
 *            defining this HFSM's hierarchy.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result _az_hfsm_init(
    _az_hfsm* h,
    _az_hfsm_state_handler root_state,
    _az_hfsm_get_parent get_parent_func);

/**
 * @brief Transition to a peer state.
 *
 * @note  Calling this as a result of AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE is supported.
 *        The application developer is responsible with ensuring the hierarchy is properly defined
 *        and no invalid calls are made.
 *
 * @param[in] h The #_az_hfsm to use for this call.
 * @param[in] source_state The source state.
 * @param[in] destination_state The destination state.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result _az_hfsm_transition_peer(
    _az_hfsm* h,
    _az_hfsm_state_handler source_state,
    _az_hfsm_state_handler destination_state);

/**
 * @brief Transition to a sub state.
 *
 * @note  Calling this as a result of AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE is supported.
 *        The application developer is responsible with ensuring the hierarchy is properly defined
 *        and no invalid calls are made.
 *
 * @param[in] h The #_az_hfsm to use for this call.
 * @param[in] source_state The source state.
 * @param[in] destination_state The destination state.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result _az_hfsm_transition_substate(
    _az_hfsm* h,
    _az_hfsm_state_handler source_state,
    _az_hfsm_state_handler destination_state);

/**
 * @brief Transition to a super state.
 *
 * @note  Calling this as a result of AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE is supported.
 *        The application developer is responsible with ensuring the hierarchy is properly defined
 *        and no invalid calls are made.
 *
 * @param[in] h The #_az_hfsm to use for this call.
 * @param[in] source_state The source state.
 * @param[in] destination_state The destination state.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success.
 */
AZ_NODISCARD az_result _az_hfsm_transition_superstate(
    _az_hfsm* h,
    _az_hfsm_state_handler source_state,
    _az_hfsm_state_handler destination_state);

/**
 * @brief Synchronously sends an event to a HFSM object.
 *
 * @note The lifetime of the event's `data` argument must be guaranteed until this function returns.
 *       All state handlers related to this event will execute on the current stack. In most cases
 *       it is recommended that a queue together with a message pump is used as an intermediary to
 *       avoid recursive calls.
 *
 * @param[in] h The #_az_hfsm to use for this call.
 * @param[in] event The event being sent.
 * 
 * @return An #az_result value indicating the result of the operation.
 * @retval #AZ_OK Success
 */
AZ_NODISCARD az_result _az_hfsm_send_event(_az_hfsm* h, az_event event);

#include <azure/core/_az_cfg_suffix.h>

#endif //_az_HFSM_INTERNALH
