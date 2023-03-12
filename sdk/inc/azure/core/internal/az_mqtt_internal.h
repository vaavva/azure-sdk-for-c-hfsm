// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifndef _az_MQTT_INTERNAL_H
#define _az_MQTT_INTERNAL_H

#include <azure/core/az_span.h>
#include <azure/core/az_event.h>
#include <azure/core/internal/az_event_pipeline.h>

// MQTT library handle (type defined by implementation)
typedef struct az_mqtt az_mqtt;

typedef struct
{
  /**
   * The CA Trusted Roots span interpretable by the underlying MQTT implementation.
   */
  az_span certificate_authority_trusted_roots;
  bool clean_session;
} az_mqtt_options_common;

typedef struct
{
  _az_event_pipeline* pipeline;
} az_mqtt_common;

#endif // _az_MQTT_INTERNAL_H
