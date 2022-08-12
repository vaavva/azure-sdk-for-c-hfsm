/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm.c
 * @brief Hierarchical Finite State Machine (HFSM) for Azure IoT Operations.
 *
 * @details Implements fault handling for Azure Device Provisioning + IoT Hub operations.
 */
#ifndef _az_IOT_RETRY_HFSM_H
#define _az_IOT_RETRY_HFSM_H

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/az_config.h>
#include <azure/core/az_hfsm.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/az_mqtt.h>

#include <azure/az_iot.h>

#include <stdbool.h>
#include <stdint.h>

#include <azure/core/_az_cfg_prefix.h>

typedef struct
{
  az_span cert;
  az_span key;
} az_hfsm_iot_x509_auth;

typedef struct
{
  az_span shared_access_key;
  az_span unencrypted_sas_buffer;
  // HFSM_DESIGN: The application must implement an AZ_HFSM_HMACSHA256_EVENT_REQ and 
  //              send a AZ_HFSM_HMACSHA256_EVENT_RSP to the state machine.
} az_hfsm_iot_sas_auth;

typedef union
{
  az_hfsm_iot_x509_auth x509;
  az_hfsm_iot_sas_auth sas;
} az_hfsm_iot_auth;

typedef enum
{
  AZ_HFSM_IOT_AUTH_SAS = 0,
  AZ_HFSM_IOT_AUTH_X509 = 1,
} az_hfsm_iot_auth_type;

typedef struct
{
  az_hfsm_iot_auth secondary_credential;
} az_hfsm_iot_retry_policy_options;

typedef struct
{
  struct
  {
    az_hfsm_policy policy;

    az_hfsm_iot_auth_type auth_type;
    az_hfsm_iot_auth primary_credential;

    // Alloc objects
    az_span username_buffer;
    az_span password_buffer;
    az_span client_id_buffer;

    az_hfsm_iot_retry_policy_options options;
  } _internal;
} az_hfsm_iot_retry_policy;

enum az_hfsm_event_type_iot_hfsm
{
  /// Connection control
  AZ_IOT_CONNECT_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_HFSM, 0),
  AZ_IOT_CONNECT_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_HFSM, 1),
  AZ_IOT_DISCONNECT_REQ = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_HFSM, 2),
  AZ_IOT_DISCONNECT_RSP = _az_HFSM_MAKE_EVENT(_az_FACILITY_IOT_HFSM, 3),
};

AZ_NODISCARD az_hfsm_iot_retry_policy_options az_hfsm_iot_retry_policy_options_default();

AZ_NODISCARD az_result az_hfsm_iot_retry_policy_initialize(
    az_hfsm_iot_retry_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* outbound_policy,
    az_hfsm_iot_auth_type auth_type,
    az_hfsm_iot_auth* primary_credential,
    az_span username_buffer,
    az_span password_buffer,
    az_span client_id_buffer,
    az_hfsm_iot_retry_policy_options const* options);

#endif //_az_IOT_RETRY_HFSM_H
