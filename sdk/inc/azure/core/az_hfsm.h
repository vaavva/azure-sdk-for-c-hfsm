/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm.h
 * @brief Definition of #az_hfsm and related types describing a Hierarchical Finite State Machine 
 *        (HFSM).
 * 
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#ifndef _az_HFSM_H
#define _az_HFSM_H

#include <stdint.h>
#include <azure/core/az_result.h>

enum
{
  _az_HFSM_CORE_ID = 1,
  _az_HFSM_IOT_ID = 2,
};

#define _az_HFSM_MAKE_EVENT(hfsm_id, code) \
  ((az_hfsm_event_type)(((uint32_t)(hfsm_id) << 16U) | (uint32_t)(code)))

/**
 * @brief The type represents event types handled by a HFSM.
 *
 * @note See the following `az_hfsm_event_type` values from various headers:
 *  - #az_hfsm_event_type_core
 *  - #az_hfsm_event_type_iot
 */
typedef int32_t az_hfsm_event_type;

/**
 * @brief Common HFSM event types.
 * 
 */
enum az_hfsm_event_type_core
{
  AZ_HFSM_EVENT_ENTRY = _az_HFSM_MAKE_EVENT(_az_HFSM_CORE_ID, 1),
  AZ_HFSM_EVENT_EXIT = _az_HFSM_MAKE_EVENT(_az_HFSM_CORE_ID, 2),
  AZ_HFSM_EVENT_ERROR = _az_HFSM_MAKE_EVENT(_az_HFSM_CORE_ID, 3),
  AZ_HFSM_EVENT_TIMEOUT = _az_HFSM_MAKE_EVENT(_az_HFSM_CORE_ID, 4),
};

/**
 * @brief The type represents possible return codes from HFSM state handlers.
 * 
 */
typedef int32_t az_hfsm_return_type;

/**
 * @brief The only allowed HFSM state return types.
 * 
 */
enum az_hfsm_return_type_core
{
  AZ_HFSM_RET_HANDLED = 0,
  AZ_HFSM_RET_HANDLE_BY_SUPERSTATE = 1,
};

/**
 * @brief The type represents an event handled by HFSM.
 * 
 */
typedef struct
{
  /**
   * @brief The event type.
   * 
   */
  az_hfsm_event_type type;

  /**
   * @brief The event data.
   * 
   */
  void* data;
} az_hfsm_event;

/**
 * @brief The generic state entry event.
 * 
 * @note The entry and exit events must not expect a `data` field.
 */
extern const az_hfsm_event AZ_HFSM_EVENT_ENTRY_event;

/**
 * @brief The generic state exit event.
 * 
 * @note The entry and exit events must not expect a `data` field.
 */
extern const az_hfsm_event AZ_HFSM_EVENT_EXIT_event;

/**
 * @brief The type representing a Hierarchical Finite State Machine (HFSM) object.
 * 
 */
typedef struct az_hfsm az_hfsm;

/**
 * @brief The function signature for all HFSM states.
 * 
 * @note All HFSM states must follow the pattern of a single `switch ((int32_t)event.type)` 
 *       statement. Code should not exist outside of the switch statement.
 */
typedef az_hfsm_return_type (*az_hfsm_state_handler)(az_hfsm* me, az_hfsm_event event);

/**
 * @brief The function signature for the function returning the parent of any given child state.
 * 
 * @note All HFSMs must define a `get_parent` function. A top-level state is mandatory.  
 *       `get_parent(top_level_state)` must always return `NULL`.
 */
typedef az_hfsm_state_handler (*az_hfsm_get_parent)(az_hfsm_state_handler child_state);

// Avoiding a circular dependency between az_hfsm, az_hfsm_state_handler and az_hfsm_get_parent.
struct az_hfsm
{
  az_hfsm_state_handler current_state;
  az_hfsm_get_parent get_parent_func;
};

az_result az_hfsm_init(
    az_hfsm* h,
    az_hfsm_state_handler initial_state,
    az_hfsm_get_parent get_parent_func);

void az_hfsm_transition_peer(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state);

void az_hfsm_transition_substate(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state);

void az_hfsm_transition_superstate(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state);

void az_hfsm_send_event(az_hfsm* h, az_hfsm_event event);

// TODO: az_hfsm_post_event - in-line with other HFSM systems (WinGUI, QT, etc) where send means sync, post means enqueue.

#endif //_az_HFSM_H
