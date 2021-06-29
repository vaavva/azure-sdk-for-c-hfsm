/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_hfsm.h
 * @brief Hierarchical Finite State Machine implementation.
 */

#ifndef _az_HFSM_H
#define _az_HFSM_H

#include <stdint.h>

/**
 * @brief hfsm event type.
 *
 */
typedef enum
{
  AZ_HFSM_ENTRY = 1,
  AZ_HFSM_EXIT = 2,
  AZ_HFSM_ERROR = 3,
  AZ_HFSM_TIMEOUT = 4,
  AZ_HFSM_EVENT_BASE = 10,
} az_hfsm_event_type;

#define AZ_HFSM_EVENT(id) ((int32_t)(AZ_HFSM_EVENT_BASE + id))

typedef enum
{
  // ASCII "SUPR"
  AZ_HFSM_RET_HANDLED = 1,
  AZ_HFSM_RET_HANDLE_BY_SUPERSTATE = 2,
} az_hfsm_return_type;

struct az_hfsm_event
{
  az_hfsm_event_type type;
  void* data;
};

typedef struct az_hfsm_event az_hfsm_event;

extern const az_hfsm_event az_hfsm_entry_event;
extern const az_hfsm_event az_hfsm_exit_event;
extern const az_hfsm_event az_hfsm_timeout_event;
extern const az_hfsm_event az_hfsm_error_event;

typedef struct az_hfsm az_hfsm;
typedef int32_t (*az_hfsm_state_handler)(az_hfsm* me, az_hfsm_event event);
typedef az_hfsm_state_handler (*az_hfsm_get_parent)(az_hfsm_state_handler child_state);

struct az_hfsm
{
  az_hfsm_state_handler current_state;
  az_hfsm_get_parent get_parent_func;
};

int32_t az_hfsm_init(
    az_hfsm* h,
    az_hfsm_state_handler initial_state,
    az_hfsm_get_parent get_parent_func);

int32_t az_hfsm_transition_peer(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state);

int32_t az_hfsm_transition_substate(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state);


int32_t az_hfsm_transition_superstate(
    az_hfsm* h,
    az_hfsm_state_handler source_state,
    az_hfsm_state_handler destination_state);

int32_t az_hfsm_send_event(az_hfsm* h, az_hfsm_event event);

// TODO: az_hfsm_post_event - in-line with other HFSM systems (WinGUI, QT, etc) where send means syncrhonous, post means enqueue.

#endif //_az_HFSM_H
