/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief Hierarchical Finite State Machine (HFSM) for Azure IoT Operations.
 * 
 * @details Implements fault handling for Azure Device Provisioning + IoT Hub operations.
 */
#ifndef _az_IOT_HFSM_H
#define _az_IOT_HFSM_H

#include <stdint.h>
#include <stdbool.h>

#include <azure/az_core.h>
#include <azure/core/az_hfsm.h>
#include <azure/iot/az_iot_common.h>

#ifndef AZ_IOT_HFSM_MIN_RETRY_DELAY_MSEC
#define AZ_IOT_HFSM_MIN_RETRY_DELAY_MSEC AZ_IOT_DEFAULT_MIN_RETRY_DELAY_MSEC
#endif 

#ifndef AZ_IOT_HFSM_MAX_RETRY_DELAY_MSEC
#define AZ_IOT_HFSM_MAX_RETRY_DELAY_MSEC AZ_IOT_DEFAULT_MAX_RETRY_DELAY_MSEC
#endif 

#ifndef AZ_IOT_HFSM_MAX_RETRY_JITTER_MSEC
#define AZ_IOT_HFSM_MAX_RETRY_JITTER_MSEC AZ_IOT_DEFAULT_MAX_RETRY_JITTER_MSEC
#endif 

#ifndef AZ_IOT_HFSM_MAX_HUB_RETRY
#define AZ_IOT_HFSM_MAX_HUB_RETRY AZ_IOT_DEFAULT_MAX_HUB_RETRY
#endif 

// TODO
#define AZ_IOT_HFSM_PROVISIONING_ENABLED

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

/**
 * @brief The generic IoT start event.
 * 
 */
extern const az_hfsm_event az_hfsm_event_az_iot_start;

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
/**
 * @brief The generic provisioning done event.
 * 
 */
extern const az_hfsm_event az_hfsm_event_az_iot_provisioning_done;
#endif

/**
 * @brief IoT error event data. Extends #az_hfsm_event_data_error.
 * 
 */
typedef struct {
  az_hfsm_event_data_error error;
  az_iot_status iot_status;
} az_iot_hfsm_event_data_error;

/**
 * @brief Initializes an IoT HFSM object.
 * 
 * @param[out] iot_hfsm The #az_hfsm to use for this call.
 * @param[in] provisioning_hfsm The #az_hfsm implementing #az_iot_provisioning_client operations.
 * @param[in] hub_hfsm The #az_hfsm implementing #az_iot_hub_client operations.
 * @return An #az_result value indicating the result of the operation.
 */
AZ_NODISCARD az_result az_iot_hfsm_initialize(
  az_iot_hfsm_type* iot_hfsm, 
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
  az_hfsm_dispatch* provisioning_hfsm, 
#endif
  az_hfsm_dispatch* hub_hfsm);

#endif //_az_IOT_HFSM_H
