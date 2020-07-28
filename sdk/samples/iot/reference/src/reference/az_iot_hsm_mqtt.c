// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/internal/az_iot_hsm.h>

#include <azure/core/_az_cfg.h>

az_iot_hsm az_iot_hsm_mqtt;

static az_result disconnected(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
