#ifndef AZ_IOT_HFSM_H
#define AZ_IOT_HFSM_H
#include <hfsm_hardcoded.h>

// Devices should always be provisioned with at least 2 credentials to prevent them
// from loosing connectivity with the cloud and firmware update systems.
#define CREDENTIAL_COUNT 2

// AzIoTHFSM-specific events.
typedef enum
{
  ERROR = HFSM_EVENT(1),
  TIMEOUT = HFSM_EVENT(2),
  AZ_IOT_PROVISIONING_START = HFSM_EVENT(3),
  AZ_IOT_HUB_START = HFSM_EVENT(4),
} az_iot_hsm_event_type;

// AZIoTHFSM-specific event data
typedef struct
{
    az_span mqtt_user_name;
    az_span password;
    void* client_certificate_information;
} az_iot_provisioning_start_data;

typedef struct
{
    az_span mqtt_user_name;
    az_span password;
    void* client_certificate_information;
} az_iot_hub_start_data;

#endif //AZ_IOT_HFSM_H
