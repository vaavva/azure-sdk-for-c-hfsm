// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file az_iot_hsm.h
 *
 * @brief Azure IoT Hierarchical State Machine.
 * https://en.wikipedia.org/wiki/UML_state_machine#Hierarchically_nested_states
 *
 * @note You MUST NOT use any symbols (macros, functions, structures, enums, etc.)
 * prefixed with an underscore ('_') directly in your application code. These symbols
 * are part of Azure SDK's internal implementation; we do not document these symbols
 * and they are subject to change in future versions of the SDK which would break your code.
 */

#define HSM_HARDCODED
//#define HSM_QUEUE

#ifdef HSM_HARDCODED
#include <azure/iot/internal/az_iot_hsm_impl_hardcoded.h>
#elif defined(HSM_QUEUE)
#include <azure/iot/internal/az_iot_hsm_impl_queue.h>
#else
#include <azure/iot/internal/az_iot_hsm_impl_stack.h>
#endif
