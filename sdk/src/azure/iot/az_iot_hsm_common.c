// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/iot/internal/az_iot_hsm.h>

#include <azure/core/_az_cfg.h>

const az_iot_hsm_event az_iot_hsm_entry_event = { AZ_IOT_HSM_ENTRY, NULL };
const az_iot_hsm_event az_iot_hsm_exit_event = { AZ_IOT_HSM_EXIT, NULL };
const az_iot_hsm_event az_iot_hsm_timeout_event = { AZ_IOT_HSM_TIMEOUT, NULL };
