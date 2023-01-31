/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_sm_provisioning_sample.c
 * @brief HFSM IoT Sample
 *
 * @details This application directly uses the two IoT pipelines. Messages received from the
 * pipelines are interpreted by a HFSM.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/az_log.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/az_iot.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/iot/internal/az_iot_hub_hfsm.h>
#include <azure/iot/internal/az_iot_provisioning_hfsm.h>
#include <azure/iot/internal/az_iot_retry_hfsm.h>

#include <azure/iot/az_iot_sm_provisioning_client.h>

#include "mosquitto.h"

static const az_span dps_endpoint
    = AZ_SPAN_LITERAL_FROM_STR("global.azure-devices-provisioning.net");
static const az_span id_scope = AZ_SPAN_LITERAL_FROM_STR("0ne00003E26");
static const az_span device_id = AZ_SPAN_LITERAL_FROM_STR("dev1-ecc");
static char hub_endpoint_buffer[120];
static az_span hub_endpoint;
static const az_span ca_path = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/rsa_baltimore_ca.pem");
static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/dev1-ecc_cert.pem");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/dev1-ecc_key.pem");

// static const az_span cert_path2 =
// AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/dev1-ecc_cert.pem"); static const az_span key_path2
// = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/dev1-ecc_key.pem");

static char client_id_buffer[64];
static char username_buffer[128];
static char password_buffer[1];
static char topic_buffer[128];
static char payload_buffer[256];

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
    case AZ_LOG_MQTT_RECEIVED_TOPIC:
      class_str = "AZ_LOG_MQTT_RECEIVED_TOPIC";
      break;
    case AZ_LOG_MQTT_RECEIVED_PAYLOAD:
      class_str = "AZ_LOG_MQTT_RECEIVED_PAYLOAD";
      break;
    case AZ_IOT_PROVISIONING_REGISTER_REQ:
      class_str = "AZ_IOT_PROVISIONING_REGISTER_REQ";
      break;
    case AZ_IOT_HUB_CONNECT_REQ:
      class_str = "AZ_IOT_HUB_CONNECT_REQ";
      break;
    case AZ_IOT_HUB_CONNECT_RSP:
      class_str = "AZ_IOT_HUB_CONNECT_RSP";
      break;
    case AZ_IOT_HUB_DISCONNECT_REQ:
      class_str = "AZ_IOT_HUB_DISCONNECT_REQ";
      break;
    case AZ_IOT_HUB_DISCONNECT_RSP:
      class_str = "AZ_IOT_HUB_DISCONNECT_RSP";
      break;
    case AZ_IOT_HUB_TELEMETRY_REQ:
      class_str = "AZ_IOT_HUB_TELEMETRY_REQ";
      break;
    case AZ_IOT_HUB_METHODS_REQ:
      class_str = "AZ_IOT_HUB_METHODS_REQ";
      break;
    case AZ_IOT_HUB_METHODS_RSP:
      class_str = "AZ_IOT_HUB_METHODS_RSP";
      break;
    default:
      class_str = NULL;
  }

  if (class_str == NULL)
  {
    printf(LOG_SDK "[\x1B[31mUNKNOWN: %x\x1B[0m] %s\n", classification, az_span_ptr(message));
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
  printf(LOG_APP "\x1B[31mPANIC!\x1B[0m\n");

  while (1)
    ;
}

static az_platform_mutex disconnect_mutex;
static az_iot_provisioning_client prov_codec;
static az_iot_sm_provisioning_client prov_client;


az_result prov_registration_status_callback(az_iot_sm_provisioning_client* client)
{
  az_iot_provisioning_client_register_response* response;
  az_iot_sm_provisioning_client_register_get_status(client, &response);

  // guaranteed that response here is an intermediate result
}

az_result prov_registraiton_result_callback(az_iot_sm_provisioning_client* client)
{
  az_iot_provisioning_client_register_response* response;
  az_iot_sm_provisioning_client_register_get_result(client, &response);

  // guaranteed that response here is the final result

  az_platform_mutex_release(&disconnect_mutex);
}

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  /* Required before calling other mosquitto functions */
  _az_RETURN_IF_FAILED(az_mqtt_init());
  printf("Using MosquittoLib %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  az_hfsm_iot_x509_auth auth = (az_hfsm_iot_x509_auth){
    .cert = cert_path1,
    .key = key_path1,
  };

  _az_RETURN_IF_FAILED(az_platform_mutex_init(&disconnect_mutex));
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&disconnect_mutex));

  _az_RETURN_IF_FAILED(az_iot_provisioning_client_init(&prov_codec, ...));
  _az_RETURN_IF_FAILED(
      az_iot_sm_provisioning_client_init(&prov_client, &prov_codec, AZ_HFSM_IOT_AUTH_X509, auth));

  _az_RETURN_IF_FAILED(az_iot_sm_provisioning_client_register(&prov_client, ))

  for (int i = 15; i > 0; i--)
  {
    _az_RETURN_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Waiting %ds        \r", i);
    fflush(stdout);
  }

  printf(LOG_APP "Main thread disconnecting...\n");
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_post_outbound_event(
      &hub_pipeline, (az_hfsm_event){ AZ_IOT_HUB_DISCONNECT_REQ, NULL }));

  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&disconnect_mutex));
  printf(LOG_APP "Done.\n");

  _az_RETURN_IF_FAILED(az_mqtt_deinit());

  return 0;
}
