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

#define TEMP_PROVISIONING
// TODO : #define TEMP_HUB

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

// Provisioning
static az_hfsm_pipeline prov_pipeline;
static az_hfsm_iot_provisioning_policy prov_policy;
static az_iot_provisioning_client prov_client;
static az_hfsm_mqtt_policy prov_mqtt_policy;

// Hub
static az_hfsm_pipeline hub_pipeline;
static az_hfsm_iot_hub_policy hub_policy;
static az_iot_hub_client hub_client;
static az_hfsm_mqtt_policy hub_mqtt_policy;

// Retry & endpoint orchestrator
static az_hfsm_iot_retry_policy retry_policy;

static az_result prov_initialize()
{
  // TODO: temporary start with DPS
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &prov_pipeline, (az_hfsm_policy*)&prov_policy, (az_hfsm_policy*)&prov_mqtt_policy));

  _az_RETURN_IF_FAILED(
      az_iot_provisioning_client_init(&prov_client, dps_endpoint, id_scope, device_id, NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_provisioning_policy_initialize(
      &prov_policy,
      &prov_pipeline,
      NULL, // TODO: temporary
      (az_hfsm_policy*)&prov_mqtt_policy,
      &prov_client,
      NULL));

  // MQTT
  az_hfsm_mqtt_policy_options mqtt_options = az_hfsm_mqtt_policy_options_default();
  mqtt_options.certificate_authority_trusted_roots = ca_path;

  _az_RETURN_IF_FAILED(az_mqtt_initialize(
      &prov_mqtt_policy, &prov_pipeline, (az_hfsm_policy*)&prov_policy, &mqtt_options));

  return AZ_OK;
}

static az_result hub_initialize()
{
  hub_endpoint = AZ_SPAN_FROM_BUFFER(hub_endpoint_buffer);

  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &hub_pipeline, (az_hfsm_policy*)&retry_policy, (az_hfsm_policy*)&hub_mqtt_policy));

  _az_RETURN_IF_FAILED(az_iot_hub_client_init(&hub_client, hub_endpoint, device_id, NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_hub_policy_initialize(
      &hub_policy,
      &hub_pipeline,
      (az_hfsm_policy*)&hub_mqtt_policy,
      (az_hfsm_policy*)&retry_policy,
      &hub_client,
      NULL));

  // MQTT
  az_hfsm_mqtt_policy_options mqtt_options = az_hfsm_mqtt_policy_options_default();
  mqtt_options.certificate_authority_trusted_roots = ca_path;

  _az_RETURN_IF_FAILED(az_mqtt_initialize(
      &hub_mqtt_policy, &hub_pipeline, (az_hfsm_policy*)&hub_policy, &mqtt_options));

  return AZ_OK;
}

static az_result initialize()
{
  _az_RETURN_IF_FAILED(prov_initialize());
  _az_RETURN_IF_FAILED(hub_initialize());
  // Retry
  az_hfsm_iot_auth primary_cred;
  primary_cred.x509 = (az_hfsm_iot_x509_auth){ .cert = cert_path1, .key = key_path1 };

  az_hfsm_iot_retry_policy_options retry_options = az_hfsm_iot_retry_policy_options_default();
  retry_options.secondary_credential.x509
      = (az_hfsm_iot_x509_auth){ .cert = cert_path2, .key = key_path2 };

  _az_RETURN_IF_FAILED(az_hfsm_iot_retry_policy_initialize(
      &retry_policy,
      &prov_pipeline,
      (az_hfsm_policy*)&prov_policy,
      AZ_HFSM_IOT_AUTH_X509,
      &primary_cred,
      AZ_SPAN_FROM_BUFFER(username_buffer),
      AZ_SPAN_FROM_BUFFER(password_buffer),
      AZ_SPAN_FROM_BUFFER(client_id_buffer),
      &retry_options));

  return AZ_OK;
}

// HFSM_TODO: Error handling intentionally missing.
int main(int argc, char* argv[])
{
  (void)argc;
  (void)argv;

  /* Required before calling other mosquitto functions */
  _az_RETURN_IF_FAILED(az_mqtt_init());
  printf("Using MosquittoLib %d\n", mosquitto_lib_version(NULL, NULL, NULL));

  az_log_set_message_callback(az_sdk_log_callback);
  az_log_set_classification_filter_callback(az_sdk_log_filter_callback);

  _az_RETURN_IF_FAILED(initialize());

  az_hfsm_iot_x509_auth auth = (az_hfsm_iot_x509_auth){
    .cert = cert_path1,
    .key = key_path1,
  };

#ifdef TEMP_PROVISIONING
  az_hfsm_iot_provisioning_register_data register_data = (az_hfsm_iot_provisioning_register_data){
    .auth = auth,
    .auth_type = AZ_HFSM_IOT_AUTH_X509,
    .client_id_buffer = AZ_SPAN_FROM_BUFFER(client_id_buffer),
    .username_buffer = AZ_SPAN_FROM_BUFFER(username_buffer),
    .password_buffer = AZ_SPAN_FROM_BUFFER(password_buffer),
    .topic_buffer = AZ_SPAN_FROM_BUFFER(topic_buffer),
    .payload_buffer = AZ_SPAN_FROM_BUFFER(payload_buffer),
    // HFSM_TODO: hub_endpoint, device_id buffer are missing.
  };
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_post_outbound_event(
      &prov_pipeline, (az_hfsm_event){ AZ_IOT_PROVISIONING_REGISTER_REQ, &register_data }));
#endif

#ifdef TEMP_HUB
  az_hfsm_iot_hub_connect_data connect_data = (az_hfsm_iot_hub_connect_data){
    .auth = auth,
    .auth_type = AZ_HFSM_IOT_AUTH_X509,
    .client_id_buffer = AZ_SPAN_FROM_BUFFER(client_id_buffer),
    .username_buffer = AZ_SPAN_FROM_BUFFER(username_buffer),
    .password_buffer = AZ_SPAN_FROM_BUFFER(password_buffer),
  };
#endif

  for (int i = 15; i > 0; i--)
  {
    _az_RETURN_IF_FAILED(az_platform_sleep_msec(1000));
    printf(LOG_APP "Waiting %ds        \r", i);
    fflush(stdout);
  }


  _az_RETURN_IF_FAILED(az_mqtt_deinit());

  return 0;
}
