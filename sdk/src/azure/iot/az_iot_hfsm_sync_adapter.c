/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_hfsm_sync_adapter.c
 * @brief Synchronous adapter for the Azure IoT state machine
 */

#include <stdint.h>
#include <azure/core/internal/az_precondition_internal.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/iot/internal/az_iot_hfsm.h>
#include <azure/iot/internal/az_iot_hfsm_sync_adapter.h>

// The retry Hierarchical Finite State Machine object.
// A single Provisioning + Hub client is supported when synchronous mode is used.
static az_iot_hfsm_type iot_hfsm;
static az_hfsm_dispatch hub_hfsm;
static iot_service_sync_function do_syncrhonous_hub;

#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
static az_hfsm_dispatch provisioning_hfsm;
static iot_service_sync_function do_syncrhonous_provisioning;
#endif


static az_hfsm_return_type azure_hub_sync(az_hfsm* me, az_hfsm_event event);
static az_hfsm_return_type azure_provisioning_sync(az_hfsm* me, az_hfsm_event event);
static az_hfsm_state_handler provisioning_and_hub_get_parent(az_hfsm_state_handler child_state)
{
  (void)child_state;
  return NULL;
}

static az_hfsm_return_type azure_hub_sync(az_hfsm* me, az_hfsm_event event)
{
  (void)me;
  int32_t ret = AZ_HFSM_RETURN_HANDLED;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;

    case AZ_HFSM_IOT_EVENT_START:
      _az_PRECONDITION_NOT_NULL(event.data);
      bool* use_secondary_credentials = (bool*)event.data;
      az_iot_hfsm_event_data_error err;

      do
      {
        err = do_syncrhonous_hub(*use_secondary_credentials);
      } while (az_result_succeeded(err.error.error_type));

      az_hfsm_send_event((az_hfsm*)(&iot_hfsm), (az_hfsm_event){ AZ_HFSM_IOT_EVENT_ERROR, &err });
      break;

    case AZ_HFSM_EVENT_EXIT:
    default:
      break;
  }

  return ret;
}

static az_hfsm_return_type azure_provisioning_sync(az_hfsm* me, az_hfsm_event event)
{
  (void)me;
  int32_t ret = AZ_HFSM_RETURN_HANDLED;

  switch ((int32_t)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;

    case AZ_HFSM_IOT_EVENT_START:
      _az_PRECONDITION_NOT_NULL(event.data);
      bool* use_secondary_credentials = (bool*)event.data;

      az_iot_hfsm_event_data_error err = do_syncrhonous_provisioning(*use_secondary_credentials);
      if (az_result_succeeded(err.error.error_type))
      {
        az_hfsm_send_event((az_hfsm*)(&iot_hfsm), az_hfsm_event_az_iot_provisioning_done);
      }
      else
      {
        az_hfsm_send_event((az_hfsm*)(&iot_hfsm), (az_hfsm_event){ AZ_HFSM_IOT_EVENT_ERROR, &err });
      }
      break;

    case AZ_HFSM_EVENT_EXIT:
    default:
      break;
  }

  return ret;
}

az_result az_iot_hfsm_sync_adapter_initialize(
#ifdef AZ_IOT_HFSM_PROVISIONING_ENABLED
    iot_service_sync_function do_syncrhonous_provisioning,
#endif
    iot_service_sync_function do_syncrhonous_hub)
{
  //TODO:
  // create queues
  // copy references to callbacks.

}

void az_iot_hfsm_sync_adapter_do_work()
{
  az_hfsm_dispatch_one(&provisioning_hfsm);
  az_hfsm_dispatch_one(&hub_hfsm);
}
