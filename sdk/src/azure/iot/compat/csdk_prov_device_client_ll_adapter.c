// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license
// information.

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

#include <azure/iot/compat/internal/az_compat_csdk.h>

struct PROV_INSTANCE_INFO_TAG
{
  az_hfsm_policy compat_client_policy;

  // Endpoint information

  // Provisioning Pipeline
  az_hfsm_pipeline prov_pipeline;
  az_hfsm_iot_provisioning_policy prov_policy;
  az_iot_provisioning_client prov_client;
  az_hfsm_mqtt_policy prov_mqtt_policy;

  az_span topic_buffer;
  az_span username_buffer;
  az_span password_buffer;
  az_span client_id_buffer;

  PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK register_callback;
  PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK reg_status_cb;
};

// ************************************* C-SDK Compat layer ************************************ //

// HFSM_DESIGN: C-SDK allows a single security provider instance.
//              This instance is used by APIs that imply the security type such as 
//              IoTHubDeviceClient_LL_CreateFromDeviceAuth.
static az_hfsm_iot_x509_auth security_provider;

int prov_dev_security_init(SECURE_DEVICE_TYPE hsm_type) {}

void prov_dev_security_deinit() {}


const PROV_DEVICE_TRANSPORT_PROVIDER* Prov_Device_MQTT_Protocol(void)
{
  return (PROV_DEVICE_TRANSPORT_PROVIDER*)0x4D515454;
}



const char* Prov_Device_LL_GetVersionString() {}

PROV_DEVICE_LL_HANDLE Prov_Device_LL_Create(
    const char* uri,
    const char* scope_id,
    PROV_DEVICE_TRANSPORT_PROVIDER_FUNCTION protocol)
{
}

void Prov_Device_LL_Destroy(PROV_DEVICE_LL_HANDLE handle) {}

PROV_DEVICE_RESULT Prov_Device_LL_SetOption(
    PROV_DEVICE_LL_HANDLE handle,
    const char* optionName,
    const void* value)
{
}

void Prov_Device_LL_DoWork(PROV_DEVICE_LL_HANDLE handle) {}

PROV_DEVICE_RESULT Prov_Device_LL_Register_Device(
    PROV_DEVICE_LL_HANDLE handle,
    PROV_DEVICE_CLIENT_REGISTER_DEVICE_CALLBACK register_callback,
    void* user_context,
    PROV_DEVICE_CLIENT_REGISTER_STATUS_CALLBACK reg_status_cb,
    void* status_user_ctext)
{
}