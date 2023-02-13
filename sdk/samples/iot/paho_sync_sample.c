/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file
 * @brief Paho sync
 *
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <azure/core/az_log.h>

#include <azure/platform/az_platform_posix.h>
#include <azure/core/az_platform.h>

#include <azure/core/internal/az_result_internal.h>

#include <azure/az_iot.h>
#include <azure/iot/az_iot_sm_provisioning_client.h>

// For Provisioning events
#include <azure/core/az_hfsm.h>
#include <azure/iot/internal/az_iot_provisioning_hfsm.h>

#include <azure/platform/az_mqtt_paho.h>

static const az_span dps_endpoint
    = AZ_SPAN_LITERAL_FROM_STR("ssl://global.azure-devices-provisioning.net:8883");
static const az_span id_scope = AZ_SPAN_LITERAL_FROM_STR("0ne00003E26");
static const az_span device_id = AZ_SPAN_LITERAL_FROM_STR("dev1-ecc");
static char hub_endpoint_buffer[120];
static az_span hub_endpoint;
static const az_span ca_path = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/rsa_baltimore_ca.pem");
static const az_span cert_path1 = AZ_SPAN_EMPTY;
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("/home/crispop/test/dev1-ecc-store.pem");

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

int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  printf(LOG_APP "Using PahoMQTT\n");

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  az_mqtt_paho mqtt_paho;
  az_mqtt_options mqtt_options = az_mqtt_options_default();
  mqtt_options.certificate_authority_trusted_roots = ca_path;
  _az_RETURN_IF_FAILED(az_mqtt_init(&mqtt_paho, NULL, NULL));

  az_iot_provisioning_client prov_codec;
  _az_RETURN_IF_FAILED(
      az_iot_provisioning_client_init(&prov_codec, dps_endpoint, id_scope, device_id, NULL));

  az_iot_sm_provisioning_client prov_client;
  az_credential_x509 cred = (az_credential_x509){
    .cert = cert_path1,
    .key = key_path1,
    .key_type = AZ_CREDENTIALS_X509_KEY_MEMORY,
  };

  _az_RETURN_IF_FAILED(az_iot_sm_provisioning_client_init(
      &prov_client, &prov_codec, (az_mqtt*)&mqtt_paho, cred, NULL, NULL));

  az_context register_context
      = az_context_create_with_expiration(&az_context_application, 30 * 1000);

  _az_RETURN_IF_FAILED(az_iot_sm_provisioning_client_register(&prov_client, &register_context));

  for (int i = 15; i > 0; i--)
  {
    // HFSM_DESIGN: There are 2 options
    // 1. We maintain a single wait_for_event function that exits with the most recent event
    // returned by the server. 0-mem copy (MQTT stack must hold the memory allocated until the next
    // `read` call).
    //
    // 2. We create separate wait_for* APIs. (deep) memory copy (topic+payload) and possible double
    // parsing will be required.

    // Can return AZ_HFSM_EVENT_TIMEOUT which is not an error.
    az_hfsm_event evt = az_iot_sm_provisioning_client_wait_for_event(&prov_client, 1000);

    switch (evt.type)
    {
      case AZ_IOT_PROVISIONING_REGISTER_RSP:
        break;
        break;

      default:
        break;
    }

    printf(LOG_APP "Waiting %ds        \r", i);
    fflush(stdout);
  }

  printf(LOG_APP "Done.\n");
  return 0;
}
