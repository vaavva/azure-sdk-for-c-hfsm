// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include <stdlib.h>

#include <azure/az_iot.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/iot/internal/az_iot_provisioning_hfsm.h>

#include <azure/core/_az_cfg.h>

static az_result root(az_hfsm* me, az_hfsm_event event);
static az_result idle(az_hfsm* me, az_hfsm_event event);
static az_result running(az_hfsm* me, az_hfsm_event event);

static az_hfsm_state_handler _get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == idle || child_state == running)
  {
    parent_state = root;
  }
  else
  {
    // Unknown state.
    az_platform_critical_error();
    parent_state = NULL;
  }

  return parent_state;
}

static az_result root(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  PROV_INSTANCE_INFO* client = (PROV_INSTANCE_INFO*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("prov2/root"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op.
      break;

    case AZ_HFSM_EVENT_EXIT:
    case AZ_HFSM_EVENT_ERROR:
    {
      az_hfsm_event_data_error* err_data = (az_hfsm_event_data_error*)event.data;
      printf(
          LOG_COMPAT "\x1B[31mERROR\x1B[0m: Prov Client %p az_result=%s (%x) hfsm=%p\n",
          me,
          az_result_string(err_data->error_type),
          err_data->error_type,
          err_data->sender_hfsm);

      break;
    }

    // Pass-through events.
    case AZ_IOT_PROVISIONING_DISCONNECT_REQ:
    case AZ_HFSM_EVENT_TIMEOUT:
      // Pass-through events.
      ret = az_hfsm_pipeline_send_outbound_event((az_hfsm_policy*)me, event);
      break;

    default:
      printf(LOG_COMPAT "UNKNOWN event! %x\n", event.type);
      az_platform_critical_error();
      break;
  }

  return ret;
}

AZ_INLINE az_result _register(PROV_INSTANCE_INFO* client)
{
  _az_PRECONDITION(client->register_request_data.initialized);

  if (client->register_request_data.reg_status_cb != NULL)
  {
    client->register_request_data.reg_status_cb(
        PROV_DEVICE_REG_STATUS_REGISTERING,
        client->register_request_data.reg_status_cb_user_context);
  }

  return az_hfsm_pipeline_send_outbound_event(
      (az_hfsm_policy*)client,
      (az_hfsm_event){ AZ_IOT_PROVISIONING_REGISTER_REQ,
                       &client->register_request_data.register_data });
}

static az_result idle(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  PROV_INSTANCE_INFO* client = (PROV_INSTANCE_INFO*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("prov2/root/idle"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP:
      _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, idle, running));
      _az_RETURN_IF_FAILED(_register(client));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _handle_register_result(
    PROV_INSTANCE_INFO* client,
    az_hfsm_iot_provisioning_register_response_data* data)
{
  PROV_DEVICE_REG_STATUS reg_status;
  PROV_DEVICE_RESULT device_result;

  switch (data->operation_status)
  {
    case AZ_IOT_PROVISIONING_STATUS_UNASSIGNED:
      reg_status = PROV_DEVICE_REG_HUB_NOT_SPECIFIED;
      device_result = PROV_DEVICE_RESULT_HUB_NOT_SPECIFIED;
      break;

    case AZ_IOT_PROVISIONING_STATUS_ASSIGNING:
      reg_status = PROV_DEVICE_REG_STATUS_ASSIGNING;
      device_result = PROV_DEVICE_RESULT_HUB_NOT_SPECIFIED;
      break;

    case AZ_IOT_PROVISIONING_STATUS_ASSIGNED:
      reg_status = PROV_DEVICE_REG_STATUS_ASSIGNED;
      device_result = PROV_DEVICE_RESULT_OK;
      break;

    case AZ_IOT_PROVISIONING_STATUS_FAILED:
      reg_status = PROV_DEVICE_REG_STATUS_ERROR;
      device_result = PROV_DEVICE_RESULT_ERROR;
      break;

    case AZ_IOT_PROVISIONING_STATUS_DISABLED:
      reg_status = PROV_DEVICE_REG_STATUS_ERROR;
      device_result = PROV_DEVICE_RESULT_DISABLED;
      break;
  }

  if (client->register_request_data.reg_status_cb != NULL)
  {
    client->register_request_data.reg_status_cb(
        reg_status, client->register_request_data.reg_status_cb_user_context);
  }

  if (client->register_request_data.register_callback != NULL)
  {
    az_span_to_str(
        client->hub_name_buffer,
        sizeof(client->hub_name_buffer),
        data->registration_state.assigned_hub_hostname);

    az_span_to_str(
        client->device_id_name_buffer,
        sizeof(client->device_id_name_buffer),
        data->registration_state.device_id);

    client->register_request_data.register_callback(
        device_result,
        client->hub_name_buffer,
        client->device_id_name_buffer,
        client->register_request_data.register_callback_user_context);
  }

  return AZ_OK;
}

static az_result running(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  PROV_INSTANCE_INFO* client = (PROV_INSTANCE_INFO*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("prov2/root/running"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP:
      // No-op.
      break;

    case AZ_IOT_PROVISIONING_REGISTER_RSP:
    {
      az_hfsm_iot_provisioning_register_response_data* data
          = (az_hfsm_iot_provisioning_register_response_data*)event.data;
      ret = _handle_register_result(client, data);
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

