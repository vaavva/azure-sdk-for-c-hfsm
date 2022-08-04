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

#include "mosquitto.h"

static const az_span dps_endpoint
    = AZ_SPAN_LITERAL_FROM_STR("global.azure-devices-provisioning.net");
static const az_span id_scope = AZ_SPAN_LITERAL_FROM_STR("0ne00003E26");
static const az_span device_id = AZ_SPAN_LITERAL_FROM_STR("dev1-ecc");
static char hub_endpoint_buffer[120];
static az_span hub_endpoint;
static const az_span ca_path = AZ_SPAN_LITERAL_FROM_STR("/home/cristian/test/rsa_baltimore_ca.pem");

static const az_span cert_path1 = AZ_SPAN_LITERAL_FROM_STR("/home/cristian/test/dev1-ecc_cert.pem");
static const az_span key_path1 = AZ_SPAN_LITERAL_FROM_STR("/home/cristian/test/dev1-ecc_key.pem");

static const az_span cert_path2 = AZ_SPAN_LITERAL_FROM_STR("/home/cristian/test/dev1-ecc_cert.pem");
static const az_span key_path2 = AZ_SPAN_LITERAL_FROM_STR("/home/cristian/test/dev1-ecc_key.pem");

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

// Pipeline
static az_hfsm_pipeline pipeline;

// Inbound policy
static az_hfsm_mqtt_policy mqtt_policy;

// Hub
static az_hfsm_iot_hub_policy hub_policy;
static az_iot_hub_client hub_client;

// Provisioning
static az_hfsm_iot_provisioning_policy prov_policy;
static az_iot_provisioning_client prov_client;

// Retry & endpoint orchestrator
static az_hfsm_iot_retry_policy retry_policy;

static az_result initialize()
{
  hub_endpoint = AZ_SPAN_FROM_BUFFER(hub_endpoint_buffer);

  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &pipeline, (az_hfsm_policy*)&retry_policy, (az_hfsm_policy*)&mqtt_policy));

  // Retry
  az_hfsm_iot_auth primary_cred;
  primary_cred.x509 = (az_hfsm_iot_x509_auth){ .cert = cert_path1, .key = key_path1 };

  az_hfsm_iot_retry_policy_options retry_options = az_hfsm_iot_retry_policy_options_default();
  retry_options.secondary_credential.x509
      = (az_hfsm_iot_x509_auth){ .cert = cert_path2, .key = key_path2 };

  _az_RETURN_IF_FAILED(az_hfsm_iot_retry_policy_initialize(
      &retry_policy,
      &pipeline,
      (az_hfsm_policy*)&prov_policy,
      AZ_HFSM_IOT_AUTH_X509,
      &primary_cred,
      &retry_options));

  // DPS
  _az_RETURN_IF_FAILED(
      az_iot_provisioning_client_init(&prov_client, dps_endpoint, id_scope, device_id, NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_provisioning_policy_initialize(
      &prov_policy,
      &pipeline,
      (az_hfsm_policy*)&hub_policy,
      (az_hfsm_policy*)&retry_policy,
      &prov_client,
      NULL));

  // HUB
  _az_RETURN_IF_FAILED(az_iot_hub_client_init(&hub_client, hub_endpoint, device_id, NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_hub_policy_initialize(
      &hub_policy,
      &pipeline,
      (az_hfsm_policy*)&mqtt_policy,
      (az_hfsm_policy*)&prov_policy,
      &hub_client,
      NULL));

  // MQTT
  az_hfsm_mqtt_policy_options mqtt_options = az_hfsm_mqtt_policy_options_default();
  mqtt_options.certificate_authority_trusted_roots = ca_path;

  _az_RETURN_IF_FAILED(
      az_mqtt_initialize(&mqtt_policy, &pipeline, (az_hfsm_policy*)&hub_policy, &mqtt_options));

  return AZ_OK;
}

// HFSM_TODO: Error handling intentionally missing.
int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  /* Required before calling other mosquitto functions */
  mosquitto_lib_init();
  printf("Using MosquittoLib %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  _az_RETURN_IF_FAILED(initialize());

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  for (int i = 0; i < 15; i++)
  {
    _az_RETURN_IF_FAILED(az_platform_sleep_msec(1000));
    printf(".");
    fflush(stdout);
  }

  mosquitto_lib_cleanup();

  return 0;
}
