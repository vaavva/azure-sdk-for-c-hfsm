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

// HFSM_TODO: Move to Core/Internal

#ifndef _az_EVENT_POLICY_HFSM_H
#define _az_EVENT_POLICY_HFSM_H

#include <azure/core/az_result.h>
#include <azure/core/az_event.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  typedef struct
  {
    az_hfsm hfsm;
    az_event_policy* policy;
  } _internal;
} az_event_policy_hfsm;



#include <azure/core/_az_cfg_suffix.h>

#endif //_az_EVENT_POLICY_HFSM_H
