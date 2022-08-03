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

const char* dps_endpoint = "global.azure-devices-provisioning.net";
const char* id_scope = "0ne00003E26";
const char* device_id = "dev1-ecc";
static char hub_endpoint[120];

void az_sdk_log_callback(az_log_classification classification, az_span message);
bool az_sdk_log_filter_callback(az_log_classification classification);
az_result initialize_provisioning_pipeline();
az_result initialize_hub_pipeline();
az_result initialize_retry();

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
    printf("AZSDK [UNKNOWN: %d] %s\n", classification, az_span_ptr(message));
  }
  else
  {
    printf("AZSDK [%s] %s\n", class_str, az_span_ptr(message));
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
  printf("APP PANIC!\n");

  while (1)
    ;
}

// Pipeline
static az_hfsm_pipeline pipeline;

// Inbound policy
static az_hfsm_mqtt_policy mqtt_policy;
static az_mqtt_options mqtt_options;

// Hub
static az_hfsm_iot_hub_policy hub_policy;
static az_iot_hub_client hub_client;

// Provisioning
static az_hfsm_iot_provisioning_policy prov_policy;
static az_iot_provisioning_client prov_client;

// Retry / orchestrator
static az_hfsm_iot_retry_policy retry_policy;

az_result initialize_retry()
{
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &prov_pipeline, (az_hfsm_policy*)&retry_policy, (az_hfsm_policy*)&mqtt_policy));

  _az_RETURN_IF_FAILED(az_hfsm_iot_retry_policy_initialize(
      &retry_policy,
      &pipeline,
      (az_hfsm_policy*)&prov_policy,
      &prov_client,
      NULL));

  _az_RETURN_IF_FAILED(az_iot_provisioning_client_init(
      &prov_client,
      az_span_create_from_str(dps_endpoint),
      az_span_create_from_str(id_scope),
      az_span_create_from_str(device_id),
      NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_provisioning_policy_initialize(
      &prov_policy,
      &prov_pipeline,
      (az_hfsm_policy*)&prov_mqtt,
      (az_hfsm_policy*)&retry_policy,
      &prov_client,
      NULL));

  mqtt_options = az_mqtt_options_default();
  mqtt_options.certificate_authority_trusted_roots
      = AZ_SPAN_FROM_STR("/home/cristian/test/rsa_baltimore_ca.pem");
  mqtt_options.client_certificate = AZ_SPAN_FROM_STR("/home/cristian/test/dev1-ecc_cert.pem");
  mqtt_options.client_private_key = AZ_SPAN_FROM_STR("/home/cristian/test/dev1-ecc_key.pem");

  return AZ_OK;
}

// HFSM_TODO: Error handling intentionally missing.
int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;
  az_result az_ret;
  /* Required before calling other mosquitto functions */
  mosquitto_lib_init();
  printf("Using MosquittoLib %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  for (int i = 0; i < 15; i++)
  {
    az_ret = az_platform_sleep_msec(1000);
    printf(".");
    fflush(stdout);
  }

  mosquitto_lib_cleanup();

  return az_result_failed(az_ret);
}
