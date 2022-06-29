#include <stdlib.h>
#include <stdio.h>

#include <azure/core/az_log.h>
#include <azure/core/az_mqtt.h>
#include "mosquitto.h"

az_mqtt_hfsm_type mqtt_client;

//HFSM_TODO: replace with az_mqtt_pipeline?
az_hfsm_dispatch feedback_client;


void az_sdk_log_callback(az_log_classification classification, az_span message)
{
  const char* class_str;

  switch(classification)
  {
    case AZ_LOG_HFSM_ENTRY:
      class_str = "HFSM_ENTRY";
      break;
    case AZ_LOG_HFSM_EXIT:
      class_str = "HFSM_EXIT";
      break;
    case AZ_LOG_HFSM_TIMEOUT:
      class_str = "HFSM_TIMEOUT";
      break;
    case AZ_LOG_HFSM_ERROR:
      class_str = "HFSM_ERROR";
      break;
    case AZ_LOG_HFSM_ERROR_DETAILS:
      class_str = "HFSM_ERROR_DETAILS";
      break;
    case AZ_HFSM_MQTT_EVENT_CONNECT_REQ:
      class_str = "AZ_HFSM_MQTT_EVENT_CONNECT_REQ";
      break;
    case AZ_HFSM_MQTT_EVENT_CONNECT_RSP:
      class_str = "AZ_HFSM_MQTT_EVENT_CONNECT_RSP";
      break;
    case AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ:
      class_str = "AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ";
      break;
    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
      class_str = "AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP";
      break;
    case AZ_HFSM_MQTT_EVENT_PUB_RECV_IND:
      class_str = "AZ_HFSM_MQTT_EVENT_PUB_RECV_IND";
      break;
    case AZ_HFSM_MQTT_EVENT_PUB_REQ:
      class_str = "AZ_HFSM_MQTT_EVENT_PUB_REQ";
      break;
    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
      class_str = "AZ_HFSM_MQTT_EVENT_PUBACK_RSP";
      break;
    case AZ_HFSM_MQTT_EVENT_SUB_REQ:
      class_str = "AZ_HFSM_MQTT_EVENT_SUB_REQ";
      break;
    case AZ_HFSM_MQTT_EVENT_SUBACK_RSP:
      class_str = "AZ_HFSM_MQTT_EVENT_SUBACK_RSP";
      break;
	case AZ_LOG_HFSM_MQTT_STACK:
	  class_str = "AZ_LOG_HFSM_MQTT_STACK";
	  break;
    default:
      class_str = NULL;
  }

  if (class_str == NULL)
  {
    printf("AZSDK [UNKNOWN: %d] %s\n", classification, az_span_ptr(message));
  }
  else
  {
    printf("AZSDK [%s] %s\n", class_str, az_span_ptr(message));
  }
}

bool az_sdk_log_filter_callback(az_log_classification classification)
{
  // Enable all logging.
  return true;
}

void az_platform_critical_error()
{
  printf("AZSDK PANIC!\n");

  while(1);
}

// HFSM_TODO: Error handling intentionally missing.
int main(int argc, char *argv[])
{
  /* Required before calling other mosquitto functions */
  mosquitto_lib_init();
  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  az_mqtt_options mqtt_options = { 
    .certificate_authority_trusted_roots = AZ_SPAN_FROM_STR("S:\\test\\rsa_baltimore_ca.pem"),
                             .client_certificate = AZ_SPAN_FROM_STR("S:\\test\\dev1-ecc_cert.pem"),
                             .client_private_key = AZ_SPAN_FROM_STR("S:\\test\\dev1-ecc_key.pem") };

  az_mqtt_initialize(
    &mqtt_client,
    &feedback_client,
    AZ_SPAN_FROM_STR("crispop-iothub1.azure-devices.net"),
    8883,
    AZ_SPAN_FROM_STR("crispop-iothub1.azure-devices.net/dev1-ecc/?api-version=2020-09-30&DeviceClientType=azsdk-c%2F1.4.0-beta.1"),
    AZ_SPAN_EMPTY,
    AZ_SPAN_FROM_STR("dev1-ecc"),
    &mqtt_options);

  az_hfsm_event connect = {
    .type = AZ_HFSM_MQTT_EVENT_CONNECT_REQ,
    .data = NULL 
  };

  az_hfsm_send_event((az_hfsm*)&mqtt_client, connect);

  while(1)
  {
  }

  mosquitto_lib_cleanup();
  return 0;
}
