// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license
// information.

/**
 * @file csdk_prov_device_client_ll_adapter.c
 * @brief C-SDK Compat implementation for the IoT Provisioning LL client.
 * @details The following APIs rely on heap allocations (malloc/free) to create objects.
 *
 */

// HFSM_TODO: This implementation is proof-of-concept only.

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

#include <azure/iot/compat/azure_prov_client/prov_device_ll_client.h>
#include <azure/iot/compat/azure_prov_client/prov_security_factory.h>
#include <azure/iot/compat/azure_prov_client/prov_transport_mqtt_client.h>
#include <azure/iot/compat/iothub_client_options.h>

#include <azure/iot/compat/internal/az_compat_csdk.h>

typedef struct
{
  az_hfsm_iot_provisioning_register_data register_data;
  bool initialized;

  PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK register_callback;
  void* register_callback_user_context;
  PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK reg_status_cb;
  void* reg_status_cb_user_context;
} az_hfsm_compat_csdk_register_req;

typedef struct PROV_INSTANCE_INFO_TAG
{
  az_hfsm_policy compat_client_policy;

  // Endpoint information
  az_span dps_endpoint;
  az_span id_scope;

  // Provisioning Pipeline
  az_hfsm_pipeline prov_pipeline;
  az_hfsm_iot_provisioning_policy prov_policy;
  az_iot_provisioning_client prov_client;
  az_hfsm_mqtt_policy prov_mqtt_policy;

  // Generated values
  char topic_buffer[AZ_IOT_MAX_TOPIC_SIZE];
  char payload_buffer[AZ_IOT_MAX_PAYLOAD_SIZE];
  char username_buffer[AZ_IOT_MAX_USERNAME_SIZE];
  char password_buffer[AZ_IOT_MAX_PASSWORD_SIZE];
  char client_id_buffer[AZ_IOT_MAX_CLIENT_ID_SIZE];

  char hub_name_buffer[AZ_IOT_MAX_HUB_NAME_SIZE];
  char device_id_name_buffer[AZ_IOT_MAX_CLIENT_ID_SIZE];

  // We support a single pending register request instead of a queue.
  az_hfsm_compat_csdk_register_req register_request_data;
} PROV_INSTANCE_INFO;

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
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("compat-csdk-prov/root"));
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
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("compat-csdk-prov/root/idle"));
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
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("compat-csdk-prov/root/running"));
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

// ************************************* C-SDK Compat layer ************************************ //

int prov_dev_security_init(SECURE_DEVICE_TYPE hsm_type)
{
  security_provider = (az_hfsm_iot_x509_auth){
    .cert = AZ_SPAN_EMPTY,
    .key = AZ_SPAN_EMPTY,
  };
}

void prov_dev_security_deinit()
{ // No-op.
}

const PROV_DEVICE_TRANSPORT_PROVIDER* Prov_Device_MQTT_Protocol(void)
{
  return (PROV_DEVICE_TRANSPORT_PROVIDER*)0x4D515454;
}

const char* Prov_Device_LL_GetVersionString() { return "2.0.0-alpha"; }

static az_result prov_initialize(PROV_INSTANCE_INFO* client)
{
  client->compat_client_policy = (az_hfsm_policy){
    .inbound = NULL,
    .outbound = (az_hfsm_policy*)&client->prov_policy,
  };

  _az_RETURN_IF_FAILED(az_hfsm_init((az_hfsm*)&client->compat_client_policy, root, _get_parent));
  _az_RETURN_IF_FAILED(
      az_hfsm_transition_substate((az_hfsm*)&client->compat_client_policy, root, idle));

  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &client->prov_pipeline,
      (az_hfsm_policy*)&client->compat_client_policy,
      (az_hfsm_policy*)&client->prov_mqtt_policy));

  // Cannot initialize client w/o registration_id. Set this to a non-empty temporary value.
  _az_RETURN_IF_FAILED(az_iot_provisioning_client_init(
      &client->prov_client, client->dps_endpoint, client->id_scope, AZ_SPAN_FROM_STR("N/A"), NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_provisioning_policy_initialize(
      &client->prov_policy,
      &client->prov_pipeline,
      &client->compat_client_policy,
      (az_hfsm_policy*)&client->prov_mqtt_policy,
      &client->prov_client,
      NULL));

  // MQTT
  _az_RETURN_IF_FAILED(az_mqtt_initialize(
      &client->prov_mqtt_policy,
      &client->prov_pipeline,
      (az_hfsm_policy*)&client->prov_policy,
      NULL));

  return AZ_OK;
}

PROV_DEVICE_LL_HANDLE Prov_Device_LL_Create(
    const char* uri,
    const char* scope_id,
    PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION protocol)
{
  _az_PRECONDITION(protocol == (PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION)Prov_Device_MQTT_Protocol);

  // HFSM_TODO: param checking throughout the file.
  PROV_INSTANCE_INFO* client = malloc(sizeof(PROV_INSTANCE_INFO));

  if (client != NULL)
  {
    memset(client, 0, sizeof(PROV_INSTANCE_INFO));

    client->dps_endpoint = az_span_create_from_str((char*)(uintptr_t)uri);
    client->id_scope = az_span_create_from_str((char*)(uintptr_t)scope_id);

    if (az_result_failed(prov_initialize(client)))
    {
      free(client);
      client = NULL;
    }
  }

  return client;
}

void Prov_Device_LL_Destroy(PROV_DEVICE_LL_HANDLE handle)
{
  // Disconnect async (disconnect will be initiated here).
  if (az_result_failed(az_hfsm_pipeline_post_outbound_event(
          &handle->prov_pipeline, (az_hfsm_event){ AZ_IOT_PROVISIONING_DISCONNECT_REQ, NULL })))
  {
    az_platform_critical_error();
  }

  free(handle);
}

PROV_DEVICE_RESULT Prov_Device_LL_SetOption(
    PROV_DEVICE_LL_HANDLE handle,
    const char* optionName,
    const void* value)
{
  int result;

  if ((handle == NULL) || (optionName == NULL) || (value == NULL))
  {
    result = PROV_DEVICE_RESULT_INVALID_ARG;
  }
  else if (!strcmp(optionName, OPTION_LOG_TRACE))
  {
    if (*(bool*)value == true)
    {
      az_log_set_message_callback(az_sdk_log_callback);
      az_log_set_classification_filter_callback(az_sdk_log_filter_callback);
    }
  }
  else if (!strcmp(optionName, OPTION_OPENSSL_ENGINE))
  {
    // HFSM_TODO: not implemented.
    result = PROV_DEVICE_RESULT_ERROR;
  }
  else if (!strcmp(optionName, OPTION_OPENSSL_PRIVATE_KEY_TYPE))
  {
    // HFSM_TODO: not implemented.
    result = PROV_DEVICE_RESULT_ERROR;
  }
  else if (!strcmp(optionName, OPTION_X509_CERT))
  {
    security_provider.cert = az_span_create_from_str((char*)(uintptr_t)value);
    result = PROV_DEVICE_RESULT_OK;
  }
  else if (!strcmp(optionName, OPTION_X509_PRIVATE_KEY))
  {
    security_provider.key = az_span_create_from_str((char*)(uintptr_t)value);
    result = PROV_DEVICE_RESULT_OK;
  }
  else if (!strcmp(optionName, OPTION_TRUSTED_CERT))
  {
    handle->prov_mqtt_policy._internal.options.certificate_authority_trusted_roots
        = az_span_create_from_str((char*)(uintptr_t)value);
    result = PROV_DEVICE_RESULT_OK;
  }
  else if (!strcmp(optionName, PROV_REGISTRATION_ID))
  {
    handle->prov_client._internal.registration_id
        = az_span_create_from_str((char*)(uintptr_t)value);
    result = PROV_DEVICE_RESULT_OK;
  }
  else
  {
    result = PROV_DEVICE_RESULT_ERROR;
  }

  return result;
}

void Prov_Device_LL_DoWork(PROV_DEVICE_LL_HANDLE handle)
{
  _az_PRECONDITION(handle->register_request_data.initialized);

  az_result ret = az_hfsm_pipeline_syncrhonous_process_loop(&handle->prov_pipeline);
  if (az_result_failed(ret))
  {
    // HFSM_TODO: Call connection_callback
    printf(LOG_COMPAT "Process Loop Failed: %s (%x)\n", az_result_string(ret), ret);
    az_platform_critical_error();
  }
}

PROV_DEVICE_RESULT Prov_Device_LL_Register_Device(
    PROV_DEVICE_LL_HANDLE handle,
    PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK register_callback,
    void* user_context,
    PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK reg_status_cb,
    void* status_user_ctext)
{
  handle->register_request_data.register_data = (az_hfsm_iot_provisioning_register_data){
    .auth = security_provider,
    .auth_type = AZ_HFSM_IOT_AUTH_X509,
    .client_id_buffer = AZ_SPAN_FROM_BUFFER(handle->client_id_buffer),
    .username_buffer = AZ_SPAN_FROM_BUFFER(handle->username_buffer),
    .password_buffer = AZ_SPAN_FROM_BUFFER(handle->password_buffer),
    .topic_buffer = AZ_SPAN_FROM_BUFFER(handle->topic_buffer),
    .payload_buffer = AZ_SPAN_FROM_BUFFER(handle->payload_buffer),
  };

  handle->register_request_data.register_callback = register_callback;
  handle->register_request_data.register_callback_user_context = user_context;
  handle->register_request_data.reg_status_cb = reg_status_cb;
  handle->register_request_data.reg_status_cb_user_context = status_user_ctext;

  handle->register_request_data.initialized = true;

  return PROV_DEVICE_RESULT_OK;
}
