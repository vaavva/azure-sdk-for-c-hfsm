/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

#ifndef _az_EVENT_H
#define _az_EVENT_H

#include <azure/core/az_result.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>


// HFSM_TODO: Events will be public for logging purposes.
#define _az_HFSM_MAKE_EVENT(hfsm_id, code) \
  ((az_event_type)(((uint32_t)(hfsm_id) << 16U) | (uint32_t)(code)))

/**
 * @brief The type represents event types.
 *
 * @note See the following `az_hfsm_event_type` values from various headers:
 *  - #az_hfsm_event_type_core
 *  - #az_hfsm_event_type_iot
 */
typedef int32_t az_event_type;

/**
 * @brief The type represents an event.
 *
 */
typedef struct
{
  /**
   * @brief The event type.
   *
   */
  az_event_type type;

  /**
   * @brief The event data.
   *
   */
  void* data;
} az_event;

#include <azure/core/_az_cfg_suffix.h>
#endif /* _az_EVENT_H */
