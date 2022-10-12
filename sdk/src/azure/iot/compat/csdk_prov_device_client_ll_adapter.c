// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license
// information.

#include <azure/iot/compat/azure_prov_client/prov_device_ll_client.h>
#include <azure/iot/compat/azure_prov_client/prov_security_factory.h>
#include <azure/iot/compat/azure_prov_client/prov_transport_mqtt_client.h>

#include <azure/iot/compat/internal/az_compat_csdk.h>


// ************************************* C-SDK Compat layer ************************************ //

const PROV_DEVICE_TRANSPORT_PROVIDER* Prov_Device_MQTT_Protocol(void)
{
  return (PROV_DEVICE_TRANSPORT_PROVIDER*)0x4D515454;
}

int prov_dev_security_init(SECURE_DEVICE_TYPE hsm_type) {}

void prov_dev_security_deinit() {}

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