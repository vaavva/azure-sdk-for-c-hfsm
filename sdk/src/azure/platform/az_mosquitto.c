// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_result_internal.h>
#include <azure/core/internal/az_span_internal.h>

#include <stdlib.h>

#include <mosquitto.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_mqtt_initialize(
  az_mqtt_hfsm_type* mqtt_hfsm,
  az_span host,
  int16_t port,
  void* tls_settings,
  az_span username,
  az_span password,
  az_span client_id)
{
  // CPOP_TODO: Preconditions


}