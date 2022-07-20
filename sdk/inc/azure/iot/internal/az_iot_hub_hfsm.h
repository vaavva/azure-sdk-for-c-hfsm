/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief Hierarchical Finite State Machine (HFSM) for Azure IoT Hub.
 * 
 * @details Implements the client for Azure IoT Hub.
 */
#ifndef _az_IOT_HUB_HFSM_H
#define _az_IOT_HUB_HFSM_H

#include <stdint.h>
#include <stdbool.h>

#include <azure/az_core.h>
#include <azure/core/az_hfsm.h>
#include <azure/iot/az_iot_common.h>

typedef struct
{
  struct
  {
    az_hfsm hfsm;
    bool use_secondary_credentials;
    int16_t retry_attempt;
    int64_t start_time_msec;
    void* timer_handle;
    az_hfsm_timer_sdk_data timer_data;
    az_hfsm_event start_event;
    az_hfsm_dispatch* hub_hfsm;
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
    az_hfsm_dispatch* provisioning_hfsm;
#endif
  } _internal;
} az_iot_hfsm_type;

/**
 * @brief Azure IoT HFSM event types.
 * 
 */
enum az_hfsm_event_type_iot
{
  /// Azure IoT error event. Data is of type #az_iot_hfsm_event_data_error.
  AZ_HFSM_IOT_EVENT_ERROR = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT, 1),

  /// Azure IoT start event. The data field is always NULL.
  AZ_HFSM_IOT_EVENT_START = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT, 2),

  /// Azure IoT provisioning complete event. The data field is always NULL.
  AZ_HFSM_IOT_EVENT_PROVISIONING_DONE = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT, 3),
};

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
/**
 * @brief The generic provisioning done event.
 * 
 */
extern const az_hfsm_event az_hfsm_event_az_iot_provisioning_done;
#endif

/**
 * @brief Initializes an IoT HFSM object.
 * 
 * @param[out] iot_hfsm The #az_hfsm to use for this call.
 * @param[in] provisioning_hfsm The #az_hfsm implementing #az_iot_provisioning_client operations.
 * @param[in] hub_hfsm The #az_hfsm implementing #az_iot_hub_client operations.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_hfsm_initialize(
  az_iot_hfsm_type* iot_hub_hfsm);


#endif //_az_IOT_HUB_HFSM_H
