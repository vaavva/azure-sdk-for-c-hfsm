/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * 
 * @brief Defines events used by Azure SDK.
 */

#ifndef _az_EVENT_H
#define _az_EVENT_H

#include <azure/core/az_result.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

#define _az_MAKE_EVENT(id, code) \
  ((az_event_type)(((uint32_t)(id) << 16U) | (uint32_t)(code)))

/**
 * @brief The type represents event types.
 */
typedef int32_t az_event_type;

/**
 * @brief The type represents an event.
 */
typedef struct
{
  /**
   * @brief The event type.
   */
  az_event_type type;

  /**
   * @brief The event data.
   */
  void* data;
} az_event;

#include <azure/core/_az_cfg_suffix.h>

#endif /* _az_EVENT_H */
