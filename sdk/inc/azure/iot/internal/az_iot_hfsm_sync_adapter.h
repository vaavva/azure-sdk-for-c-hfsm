/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm_sync_adapter.c
 * @brief Synchronous adapter for the Azure IoT state machine
 * 
 * @details This adapter provides a way to convert from synchronous IoT Provisioning and Hub
 *          operations to asynchronous HFSM events. The application must implement the PAL functions.
 *          A single Provisioning + Hub client is supported in sync mode.
 */

#ifndef _az_IOT_HFSM_SYNC_ADAPTER_H
#define _az_IOT_HFSM_SYNC_ADAPTER_H

#include "az_iot_hfsm.h"

/**
 * @brief The type of function callback performing syncrhonous Azure IoT operations (either 
 *        Provisioning or Hub operations).
 * 
 */
typedef az_iot_hfsm_event_data_error (*iot_service_sync_function)( bool use_secondary_credentials );

/**
 * @brief Initializes the syncrhonous Azure IoT adapter.
 * 
 * @param[in] do_syncrhonous_provisioning The #iot_service_sync_function performing device 
 *                                        provisioning.
 * @param[in] do_syncrhonous_hub The #iot_service_sync_function performing hub operations.
 * @return An #az_result value indicating the result of the operation.
 */
az_result az_iot_hfsm_sync_adapter_initialize(
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
    iot_service_sync_function do_syncrhonous_provisioning,
#endif
    iot_service_sync_function do_syncrhonous_hub);

/**
 * @brief Single dispatching function for the syncrhonous adapter.
 * 
 * @note This dispatcher runs operations for both az_iot_hfsm, provisioning, hub and timers.
 * 
 */
void az_iot_hfsm_sync_adapter_do_work();

#endif //_az_IOT_HFSM_SYNC_ADAPTER_H
