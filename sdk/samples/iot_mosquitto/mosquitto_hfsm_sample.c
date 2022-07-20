#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <azure/core/az_log.h>
#include <azure/core/az_mqtt.h>

#include <azure/az_iot.h>

#include "mosquitto.h"

az_mqtt_hfsm_type mqtt_client;

//HFSM_TODO: replace with az_mqtt_pipeline?
az_hfsm feedback_client;

void az_sdk_log_callback(az_log_classification classification, az_span message)
{
  const char* class_str;

  switch(classification)
  {
    case AZ_HFSM_EVENT_ENTRY:
      class_str = "HFSM_ENTRY";
      break;
    case AZ_HFSM_EVENT_EXIT:
      class_str = "HFSM_EXIT";
      break;
    case AZ_HFSM_EVENT_TIMEOUT:
      class_str = "HFSM_TIMEOUT";
      break;
    case AZ_HFSM_EVENT_ERROR:
      class_str = "HFSM_ERROR";
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

// Feedback client
static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;

  switch (event.type)
  {
    case AZ_HFSM_MQTT_EVENT_CONNECT_RSP:
      az_hfsm_mqtt_connect_data* connack_data = (az_hfsm_mqtt_connect_data*)event.data;
      printf("APP: CONNACK REASON=%d\n", connack_data->connack_reason);

      int32_t sub_id;

      az_hfsm_send_event((az_hfsm*)&mqtt_client, (az_hfsm_event){
        AZ_HFSM_MQTT_EVENT_SUB_REQ, 
        &(az_hfsm_mqtt_sub_data){
          .topic_filter = AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC),
          .id = &sub_id,
          .qos = 0}});

      printf("APP: SUB mID = %d\n", sub_id);
      break;
      
    case AZ_HFSM_MQTT_EVENT_SUBACK_RSP:
      printf("APP: Subscribed\n");
      break;

    case AZ_HFSM_MQTT_EVENT_PUB_RECV_IND:
      az_hfsm_mqtt_pub_data* pub_data = (az_hfsm_mqtt_pub_data*)event.data;
      printf("APP: RECEIVED: qos=%d topic=[%s]\n", pub_data->qos, az_span_ptr(pub_data->topic));
      break;

    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
      printf("APP: PUBACK\n");
      break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
      az_hfsm_mqtt_disconnect_data* disconnect_data = (az_hfsm_mqtt_disconnect_data*)event.data;
      printf("APP: DISCONNECT reason=%d\n", disconnect_data->disconnect_reason);
      break;

    case AZ_HFSM_EVENT_ERROR:
      az_hfsm_event_data_error* err_data = (az_hfsm_event_data_error*)event.data;
      printf("APP: MQTT CLIENT ERROR: [AZ_RESULT:] %d\n", err_data->error_type);

    default:
      //NO-OP.
      break;
  }

  return ret;
}

static az_hfsm_state_handler get_parent(az_hfsm_state_handler child_state)
{
  return NULL;
}

// HFSM_TODO: Error handling intentionally missing.
int main(int argc, char *argv[])
{
  /* Required before calling other mosquitto functions */
  mosquitto_lib_init();
  printf("Using MosquittoLib %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  // Feedback: the HFSM used by the MQTT client to communicate results.
  az_hfsm_init(&feedback_client, root, get_parent);

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

  az_hfsm_send_event((az_hfsm*)&mqtt_client, (az_hfsm_event){AZ_HFSM_MQTT_EVENT_CONNECT_REQ, NULL});

  Sleep(15*1000);

  az_hfsm_send_event((az_hfsm*)&mqtt_client, (az_hfsm_event){AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ, NULL});

  Sleep(1000);

  mosquitto_lib_cleanup();
  return 0;
}
