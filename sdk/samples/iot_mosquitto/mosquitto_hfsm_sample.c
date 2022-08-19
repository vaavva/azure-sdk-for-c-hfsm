#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/az_log.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/az_iot.h>

#include "mosquitto.h"

static az_hfsm_pipeline pipeline;
static az_hfsm_policy feedback_policy;
static az_hfsm_mqtt_policy mqtt_client;

void az_sdk_log_callback(az_log_classification classification, az_span message);
bool az_sdk_log_filter_callback(az_log_classification classification);

#define LOG_APP "\x1B[34mAPP: \x1B[0m"
#define LOG_SDK "\x1B[33mSDK: \x1B[0m"

void az_sdk_log_callback(az_log_classification classification, az_span message)
{
  const char* class_str;

  switch (classification)
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
    printf(LOG_SDK "[\x1B[31mUNKNOWN: %d\x1B[0m] %s\n", classification, az_span_ptr(message));
  }
  else if (classification == AZ_HFSM_EVENT_ERROR)
  {
    printf(LOG_SDK "[\x1B[31m%s\x1B[0m] %s\n", class_str, az_span_ptr(message));
  }
  else
  {
    printf(LOG_SDK "[\x1B[35m%s\x1B[0m] %s\n", class_str, az_span_ptr(message));
  }
}

bool az_sdk_log_filter_callback(az_log_classification classification)
{
  (void)classification;
  // Enable all logging.
  return true;
}

void az_platform_critical_error()
{
  printf(LOG_APP "PANIC!\n");

  while (1)
    ;
}

static az_hfsm_mqtt_sub_data sub_data;
static az_platform_mutex disconnect_mutex;

// Feedback client
static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event)
{
  az_hfsm_policy* this_policy = (az_hfsm_policy*)me;

  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  az_result az_ret;

  switch (event.type)
  {
    case AZ_HFSM_MQTT_EVENT_CONNECT_RSP:
    {
      az_hfsm_mqtt_connack_data* connack_data = (az_hfsm_mqtt_connack_data*)event.data;
      printf(LOG_APP "CONNACK REASON=%d\n", connack_data->connack_reason);

      // This API is called here only for exemplification. In certain zero-copy implementations,
      // the underlying MQTT stack will re-used pooled network packets. In that case, the
      // application will be given `az_span` structures with maximum sizes.
      // The application must use az_span_copy:
      //      az_span_copy(
      //       sub_data.topic_filter,
      //       AZ_SPAN_FROM_STR(AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC));
      //       sub_data.qos = 0;
      //       sub_data.id = &mid;
      az_ret = az_mqtt_sub_data_create(&sub_data);
      sub_data = (az_hfsm_mqtt_sub_data){ .topic_filter = AZ_SPAN_FROM_STR(
                                              AZ_IOT_HUB_CLIENT_METHODS_SUBSCRIBE_TOPIC),
                                          .out_id = 0,
                                          .qos = 0 };

      az_hfsm_send_event(
          (az_hfsm*)&mqtt_client, (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_SUB_REQ, &sub_data });

      printf(LOG_APP "SUB mID = %d\n", sub_data.out_id);
      break;
    }

    case AZ_HFSM_MQTT_EVENT_SUBACK_RSP:
      printf(LOG_APP "Subscribed\n");

      az_ret = az_mqtt_sub_data_destroy(&sub_data);
      break;

    case AZ_HFSM_MQTT_EVENT_PUB_RECV_IND:
    {
      az_hfsm_mqtt_recv_data* recv_data = (az_hfsm_mqtt_recv_data*)event.data;
      printf(
          LOG_APP "RECEIVED: qos=%d topic=[%s]\n", recv_data->qos, az_span_ptr(recv_data->topic));
      break;
    }

    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
      printf(LOG_APP "PUBACK\n");
      break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
    {
      az_hfsm_mqtt_disconnect_data* disconnect_data = (az_hfsm_mqtt_disconnect_data*)event.data;
      printf(
          LOG_APP "DISCONNECT reason=%d - %s\n",
          disconnect_data->disconnect_reason,
          disconnect_data->disconnect_requested ? "app-requested" : "unexpected");
      az_ret = az_platform_mutex_release(&disconnect_mutex);
      break;
    }

    case AZ_HFSM_EVENT_ERROR:
    {
      az_hfsm_event_data_error* err_data = (az_hfsm_event_data_error*)event.data;
      printf(LOG_APP "MQTT CLIENT ERROR: [AZ_RESULT:] %d\n", err_data->error_type);
      break;
    }

    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      break;

    default:
      az_hfsm_send_event((az_hfsm*)this_policy->outbound, event);
      break;
  }

  if (az_result_failed(az_ret))
  {
    az_platform_critical_error();
  }

  return ret;
}

static az_hfsm_state_handler get_parent(az_hfsm_state_handler child_state)
{
  (void)child_state;
  return NULL;
}

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  /* Required before calling other mosquitto functions */
  mosquitto_lib_init();
  printf("Using MosquittoLib %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  feedback_policy = (az_hfsm_policy){ .inbound = NULL, .outbound = (az_hfsm_policy*)&mqtt_client };
  _az_RETURN_IF_FAILED(az_hfsm_init((az_hfsm*)&feedback_policy, root, get_parent));

  // Feedback: the HFSM used by the MQTT client to communicate results.
  az_hfsm_mqtt_policy_options mqtt_options = az_hfsm_mqtt_policy_options_default();
  mqtt_options.certificate_authority_trusted_roots
      = AZ_SPAN_FROM_STR("/home/cristian/test/rsa_baltimore_ca.pem");

  _az_RETURN_IF_FAILED(az_mqtt_initialize(&mqtt_client, &pipeline, &feedback_policy, &mqtt_options));
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(&pipeline, &feedback_policy, (az_hfsm_policy*)&mqtt_client));

  _az_RETURN_IF_FAILED(az_platform_mutex_init(&disconnect_mutex));

  az_hfsm_mqtt_connect_data connect_data = (az_hfsm_mqtt_connect_data){
    .host = AZ_SPAN_FROM_STR("crispop-iothub1.azure-devices.net"),
    .port = 8883,
    .username = AZ_SPAN_FROM_STR("crispop-iothub1.azure-devices.net/dev1-ecc/"
                                 "?api-version=2020-09-30&DeviceClientType=azsdk-c%2F1.4.0-"
                                 "beta.1"),
    .password = AZ_SPAN_EMPTY,
    .client_id = AZ_SPAN_FROM_STR("dev1-ecc"),
    .client_certificate = AZ_SPAN_FROM_STR("/home/cristian/test/dev1-ecc_cert.pem"),
    .client_private_key = AZ_SPAN_FROM_STR("/home/cristian/test/dev1-ecc_key.pem"),
  };

  _az_RETURN_IF_FAILED(az_hfsm_pipeline_post_outbound_event(
      &pipeline, (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_CONNECT_REQ, &connect_data }));

  for (int i = 15; i > 0; i--)
  {
    _az_RETURN_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Waiting %ds        \r", i);
    fflush(stdout);
  }

  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&disconnect_mutex));
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_post_outbound_event(
      &pipeline, (az_hfsm_event){ AZ_HFSM_MQTT_EVENT_DISCONNECT_REQ, NULL }));

  _az_RETURN_IF_FAILED(az_platform_sleep_msec(1000));

  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&disconnect_mutex));
  _az_RETURN_IF_FAILED(az_platform_mutex_destroy(&disconnect_mutex));

  return mosquitto_lib_cleanup();
}
