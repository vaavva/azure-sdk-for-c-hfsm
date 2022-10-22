/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file mosquitto_iot_sample.c
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

// HFSM Advanced Application
static az_hfsm_policy app_policy;
static az_platform_mutex disconnect_mutex;

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

static az_result prov_initialize()
{
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &prov_pipeline, (az_hfsm_policy*)&app_policy, (az_hfsm_policy*)&prov_mqtt_policy));

  _az_RETURN_IF_FAILED(
      az_iot_provisioning_client_init(&prov_client, dps_endpoint, id_scope, device_id, NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_provisioning_policy_initialize(
      &prov_policy,
      &prov_pipeline,
      &app_policy,
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
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &hub_pipeline, (az_hfsm_policy*)&app_policy, (az_hfsm_policy*)&hub_mqtt_policy));

  _az_RETURN_IF_FAILED(az_iot_hub_client_init(&hub_client, hub_endpoint, device_id, NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_hub_policy_initialize(
      &hub_policy,
      &hub_pipeline,
      &app_policy,
      (az_hfsm_policy*)&hub_mqtt_policy,
      &hub_client,
      NULL));

  // MQTT
  az_hfsm_mqtt_policy_options mqtt_options = az_hfsm_mqtt_policy_options_default();
  mqtt_options.certificate_authority_trusted_roots = ca_path;

  _az_RETURN_IF_FAILED(az_mqtt_initialize(
      &hub_mqtt_policy, &hub_pipeline, (az_hfsm_policy*)&hub_policy, &mqtt_options));

  return AZ_OK;
}

static az_hfsm_state_handler get_parent(az_hfsm_state_handler child_state)
{
  (void)child_state;
  return NULL;
}

static bool provisioned = false;

// Application - single-level HFSM
static az_result root(az_hfsm* me, az_hfsm_event event)
{
  az_hfsm_policy* this_policy = (az_hfsm_policy*)me;

  az_result ret = AZ_OK;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      break;

    case AZ_HFSM_EVENT_EXIT:
    case AZ_HFSM_EVENT_ERROR:
    {
      az_hfsm_event_data_error* err_data = (az_hfsm_event_data_error*)event.data;
      printf(
          LOG_APP "\x1B[31mERROR\x1B[0m: [AZ_RESULT:] %x HFSM: %p\n",
          err_data->error_type,
          err_data->origin);
      break;
    }

    case AZ_IOT_PROVISIONING_REGISTER_RSP:
    {
      az_hfsm_iot_provisioning_register_response_data* data
          = (az_hfsm_iot_provisioning_register_response_data*)event.data;

      if (data->operation_status == AZ_IOT_PROVISIONING_STATUS_ASSIGNED)
      {
        printf(
            LOG_APP "DPS: Registration successful: HUB=%.*s, DeviceID=%.*s\n",
            az_span_size(data->registration_state.assigned_hub_hostname),
            az_span_ptr(data->registration_state.assigned_hub_hostname),
            az_span_size(data->registration_state.device_id),
            az_span_ptr(data->registration_state.device_id));

        hub_endpoint = AZ_SPAN_FROM_BUFFER(hub_endpoint_buffer);
        az_span_copy(hub_endpoint, data->registration_state.assigned_hub_hostname);
        hub_endpoint = az_span_slice(
            hub_endpoint, 0, az_span_size(data->registration_state.assigned_hub_hostname));

        if (az_result_failed(hub_initialize()))
        {
          az_platform_critical_error();
        }

        // Wait for DPS disconnect.
        provisioned = true;
      }
      else
      {
        printf(
            LOG_APP "DPS: Registration \x1B[31mfailed\x1B[0m: TrackingID=%.*s, Error=%.*s\n",
            az_span_size(data->registration_state.error_tracking_id),
            az_span_ptr(data->registration_state.error_tracking_id),
            az_span_size(data->registration_state.error_message),
            az_span_ptr(data->registration_state.error_message));
      }
    }
    break;

    case AZ_HFSM_MQTT_EVENT_DISCONNECT_RSP:
    {
      if (provisioned)
      {
        // Switch the outbound pipeline to Hub now.
        this_policy->outbound = (az_hfsm_policy*)&hub_policy;

        az_hfsm_iot_x509_auth auth = (az_hfsm_iot_x509_auth){
          .cert = cert_path1,
          .key = key_path1,
        };

        az_hfsm_iot_hub_connect_data connect_data = (az_hfsm_iot_hub_connect_data){
          .auth = auth,
          .auth_type = AZ_HFSM_IOT_AUTH_X509,
          .client_id_buffer = AZ_SPAN_FROM_BUFFER(client_id_buffer),
          .username_buffer = AZ_SPAN_FROM_BUFFER(username_buffer),
          .password_buffer = AZ_SPAN_FROM_BUFFER(password_buffer),
        };

        ret = az_hfsm_pipeline_send_outbound_event(
            this_policy, (az_hfsm_event){ AZ_IOT_HUB_CONNECT_REQ, &connect_data });
      }
    }
    break;

    case AZ_IOT_HUB_CONNECT_RSP:
    {
      printf(LOG_APP "HUB: Connected\n");

      az_hfsm_iot_hub_telemetry_data telemetry_data = (az_hfsm_iot_hub_telemetry_data){
        .data = AZ_SPAN_LITERAL_FROM_STR("\000\001\002\003"),
        .out_packet_id = 0,
        .topic_buffer = AZ_SPAN_FROM_BUFFER(topic_buffer),
        .properties = NULL,
      };

      ret = az_hfsm_pipeline_send_outbound_event(
          this_policy, (az_hfsm_event){ AZ_IOT_HUB_TELEMETRY_REQ, &telemetry_data });
    }
    break;

    case AZ_IOT_HUB_METHODS_REQ:
    {
      az_hfsm_iot_hub_method_request_data* method_req
          = (az_hfsm_iot_hub_method_request_data*)event.data;
      printf(
          LOG_APP "HUB: Method received [%.*s]\n",
          az_span_size(method_req->name),
          az_span_ptr(method_req->name));

      az_hfsm_iot_hub_method_response_data method_rsp
          = (az_hfsm_iot_hub_method_response_data){ .status = 0,
                                                    .request_id = method_req->request_id,
                                                    .topic_buffer
                                                    = AZ_SPAN_FROM_BUFFER(topic_buffer),
                                                    .payload = AZ_SPAN_EMPTY };

      ret = az_hfsm_pipeline_send_outbound_event(
          this_policy, (az_hfsm_event){ AZ_IOT_HUB_METHODS_RSP, &method_rsp });
    }
    break;

    case AZ_IOT_HUB_DISCONNECT_RSP:
      printf(LOG_APP "HUB: Disconnected\n");
      if (az_result_failed(az_platform_mutex_release(&disconnect_mutex)))
      {
        az_platform_critical_error();
      }
      break;

    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
    {
      az_hfsm_mqtt_puback_data* puback = (az_hfsm_mqtt_puback_data*)event.data;
      printf(LOG_APP "MQTT: PUBACK ID=%d\n", puback->id);
    }
    break;

    // Pass-through events.
    case AZ_IOT_PROVISIONING_REGISTER_REQ:
    case AZ_IOT_PROVISIONING_DISCONNECT_REQ:
    case AZ_IOT_HUB_DISCONNECT_REQ:
    case AZ_HFSM_EVENT_TIMEOUT:
      // Pass-through provisioning events.
      ret = az_hfsm_pipeline_send_outbound_event((this_policy, event);
      break;

    default:
      printf(LOG_APP "UNKNOWN event! %x\n", event.type);
      break;
  }

  return ret;
}

static az_result initialize()
{
  app_policy = (az_hfsm_policy){
    .inbound = NULL,
    .outbound = (az_hfsm_policy*)&prov_policy,
  };

  _az_RETURN_IF_FAILED(az_hfsm_init((az_hfsm*)&app_policy, root, get_parent));

  _az_RETURN_IF_FAILED(prov_initialize());
  //_az_RETURN_IF_FAILED(hub_initialize());

  return AZ_OK;
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

  _az_RETURN_IF_FAILED(initialize());

  az_hfsm_iot_x509_auth auth = (az_hfsm_iot_x509_auth){
    .cert = cert_path1,
    .key = key_path1,
  };

  _az_RETURN_IF_FAILED(az_platform_mutex_init(&disconnect_mutex));
  _az_RETURN_IF_FAILED(az_platform_mutex_acquire(&disconnect_mutex));

  // HFSM_DESIGN: declarative API
  az_hfsm_iot_provisioning_register_data register_data = (az_hfsm_iot_provisioning_register_data){
    .auth = auth,
    .auth_type = AZ_HFSM_IOT_AUTH_X509, // HFSM_DESIGN: dynamic typing
    .client_id_buffer = AZ_SPAN_FROM_BUFFER(client_id_buffer),
    .username_buffer = AZ_SPAN_FROM_BUFFER(username_buffer),
    .password_buffer = AZ_SPAN_FROM_BUFFER(password_buffer),
    .topic_buffer = AZ_SPAN_FROM_BUFFER(topic_buffer),
    .payload_buffer = AZ_SPAN_FROM_BUFFER(payload_buffer),
  };
  _az_RETURN_IF_FAILED(az_hfsm_pipeline_post_outbound_event(
      &prov_pipeline, (az_hfsm_event){ AZ_IOT_PROVISIONING_REGISTER_REQ, &register_data }));

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
