// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(default : 4201)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <az_iot_provisioning_client.h>
#include <az_result.h>
#include <az_span.h>

// TODO: #564 - Remove the use of the _az_cfh.h header in samples.
#include <_az_cfg.h>

// Service information
#define GLOBAL_PROVISIONING_ENDPOINT_DEFAULT "ssl://global.azure-devices-provisioning.net:8883"
#define GLOBAL_PROVISIONING_ENDPOINT_ENV "AZ_IOT_GLOBAL_PROVISIONING_ENDPOINT"
#define ID_SCOPE_ENV "AZ_IOT_ID_SCOPE"

// Device information
#define REGISTRATION_ID_ENV "AZ_IOT_REGISTRATION_ID"

// AZ_IOT_DEVICE_X509_CERT_PEM_FILE is the path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define DEVICE_X509_CERT_PEM_FILE "AZ_IOT_DEVICE_X509_CERT_PEM_FILE"

// AZ_IOT_DEVICE_X509_TRUST_PEM_FILE is the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define DEVICE_X509_TRUST_PEM_FILE "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE"

static char global_provisioning_endpoint[256] = { 0 };
static char id_scope[16] = { 0 };
static char registration_id[256] = { 0 };
static char x509_cert_pem_file[256] = { 0 };
static char x509_trust_pem_file[256] = { 0 };

static az_iot_provisioning_client provisioning_client;
static MQTTClient mqtt_client;

static void sleep_seconds(uint32_t seconds)
{
#ifdef _WIN32
  Sleep((DWORD)seconds * 1000);
#else
  sleep(seconds);
#endif
}

static az_result null_terminate_string(az_span span)
{
  int32_t len = az_span_length(span);
  if (len >= (int32_t)az_span_capacity(span))
  {
    return AZ_ERROR_INSUFFICIENT_SPAN_CAPACITY;
  }

  char* buffer = (char*)az_span_ptr(span);
  buffer[len] = '\0';

  return AZ_OK;
}

static az_result read_configuration_entry(
    const char* name,
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value)
{
  printf("%s = ", name);
  char* env = getenv(env_name);

  if (env != NULL)
  {
    printf("%s\n", hide_value ? "***" : env);
    az_span env_span = az_span_from_str(env);
    AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(buffer, az_span_length(env_span));
    *out_value = az_span_copy(buffer, env_span);
  }
  else if (default_value != NULL)
  {
    printf("%s\n", default_value);
    az_span default_span = az_span_from_str(default_value);
    AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(buffer, az_span_length(default_span));
    *out_value = az_span_copy(buffer, default_span);
  }
  else
  {
    printf("(missing) Please set the %s environment variable.\n", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

static az_result read_configuration_and_init_client()
{
  az_span endpoint_span = AZ_SPAN_LITERAL_FROM_BUFFER(global_provisioning_endpoint);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "Global Device Endpoint",
      GLOBAL_PROVISIONING_ENDPOINT_ENV,
      GLOBAL_PROVISIONING_ENDPOINT_DEFAULT,
      false,
      endpoint_span,
      &endpoint_span));

  az_span id_scope_span = AZ_SPAN_LITERAL_FROM_BUFFER(id_scope);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "ID_Scope", ID_SCOPE_ENV, NULL, false, id_scope_span, &id_scope_span));

  az_span registration_id_span = AZ_SPAN_LITERAL_FROM_BUFFER(registration_id);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "Registration ID",
      REGISTRATION_ID_ENV,
      NULL,
      false,
      registration_id_span,
      &registration_id_span));

  az_span cert = AZ_SPAN_LITERAL_FROM_BUFFER(x509_cert_pem_file);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "X509 Certificate PEM Store File", DEVICE_X509_CERT_PEM_FILE, NULL, false, cert, &cert));

  az_span trusted = AZ_SPAN_LITERAL_FROM_BUFFER(x509_trust_pem_file);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      "X509 Trusted PEM Store File", DEVICE_X509_TRUST_PEM_FILE, "", false, trusted, &trusted));

  AZ_RETURN_IF_FAILED(az_iot_provisioning_client_init(
      &provisioning_client, endpoint_span, id_scope_span, registration_id_span, NULL));

  return AZ_OK;
}

static int connect_device()
{
  int rc;

  MQTTClient_SSLOptions mqtt_ssl_options = MQTTClient_SSLOptions_initializer;
  MQTTClient_connectOptions mqtt_connect_options = MQTTClient_connectOptions_initializer;

  char username[128];
  az_span username_span = AZ_SPAN_LITERAL_FROM_BUFFER(username);
  if ((rc = az_iot_provisioning_client_user_name_get(
           &provisioning_client, username_span, &username_span))
      != AZ_OK)
  {
    printf("Failed to get MQTT username, return code %d\n", rc);
    return rc;
  }
  
  if ((rc = null_terminate_string(username_span)) != AZ_OK)
  {
    printf("Failed to get MQTT username, return code %d\n", rc);
    return rc;
  }

  username[az_span_length(username_span)] = '\0';

  mqtt_connect_options.username = (char*)az_span_ptr(username_span);
  mqtt_connect_options.password = NULL; // Using X509 Client Certificate authentication.

  mqtt_ssl_options.keyStore = (char*)x509_cert_pem_file;
  if (*x509_trust_pem_file != '\0')
  {
    mqtt_ssl_options.trustStore = (char*)x509_trust_pem_file;
  }

  mqtt_connect_options.ssl = &mqtt_ssl_options;

  if ((rc = MQTTClient_connect(mqtt_client, &mqtt_connect_options)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to connect, return code %d\n", rc);
    return rc;
  }

  return 0;
}

static int subscribe()
{
  int rc;

  char topic_filter[128];
  az_span topic_filter_span = AZ_SPAN_LITERAL_FROM_BUFFER(topic_filter);
  if ((rc = az_iot_provisioning_client_register_subscribe_topic_filter_get(
           &provisioning_client, topic_filter_span, &topic_filter_span))
      != AZ_OK)

  {
    printf("Failed to get MQTT SUB topic filter, return code %d\n", rc);
    return rc;
  }

  if (az_span_length(topic_filter_span) >= (int32_t)sizeof(topic_filter))
  {
    printf("Failed to get MQTT username, return code %d\n", rc);
    return rc;
  }

  topic_filter[az_span_length(topic_filter_span)] = '\0';

  if ((rc = MQTTClient_subscribe(mqtt_client, topic_filter, 1)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to subscribe, return code %d\n", rc);
    return rc;
  }

  return 0;
}

static int register_device()
{
  int rc;
  MQTTClient_message pubmsg = MQTTClient_message_initializer;

  char topic[128];
  az_span topic_span = AZ_SPAN_LITERAL_FROM_BUFFER(topic);

  if ((rc = az_iot_provisioning_client_register_publish_topic_get(
           &provisioning_client, topic_span, &topic_span))
      != AZ_OK)

  {
    printf("Failed to get MQTT PUB register topic, return code %d\n", rc);
    return rc;
  }

  if ((rc = null_terminate_string(topic_span)) != AZ_OK)
  {
    printf("Failed to get MQTT PUB register topic, return code %d\n", rc);
    return rc;
  }

  pubmsg.payload = NULL; // empty payload.
  pubmsg.payloadlen = 0;
  pubmsg.qos = 1;
  pubmsg.retained = 0;

  if ((rc = MQTTClient_publishMessage(mqtt_client, topic, &pubmsg, NULL)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to publish register request, return code %d\n", rc);
    return rc;
  }

  return 0;
}

static void print_buffer(az_span span)
{
  char* buffer = (char*)az_span_ptr(span);
  for (int i = az_span_length(span); i > 0; i--)
  {
    putchar(*buffer++);
  }

  putchar('\n');
}

static int get_operation_status()
{
  int rc;
  bool registration_complete = false;
  char* topicName;
  int topicLen;
  MQTTClient_message* message;

  while (!registration_complete)
  {
    if ((rc = MQTTClient_receive(mqtt_client, &topicName, &topicLen, &message, 60 * 1000))
        != MQTTCLIENT_SUCCESS)
    {
      printf("Failed to receive message, return code %d\n", rc);
      return rc;
    }

    if (topicLen == 0)
    {
      // The length of the topic if there are one more NULL characters embedded in topicName,
      // otherwise topicLen is 0.
      topicLen = (int)strlen(topicName);
    }

    printf("Message arrived\n");
    printf("     topic (%d): %s\n", topicLen, topicName);
    printf("   message (%d): ", message->payloadlen);
    print_buffer(az_span_init(message->payload, message->payloadlen, message->payloadlen));

    az_iot_provisioning_client_register_response response;
    char query_topic[256];
    az_span query_span = AZ_SPAN_LITERAL_FROM_BUFFER(query_topic);

    if (az_failed(
            rc = az_iot_provisioning_client_received_topic_payload_parse(
                &provisioning_client,
                az_span_init((uint8_t*)topicName, topicLen, topicLen),
                az_span_init((uint8_t*)message->payload, message->payloadlen, message->payloadlen),
                &response)))
    {
      printf("Message from unknown topic, return code %d\n", rc);
      return rc;
    }

    if (az_span_is_content_equal(response.registration_state, AZ_SPAN_FROM_STR("assigning")))
    {
      if (az_failed(
              rc = az_iot_provisioning_client_get_operation_status_publish_topic_get(
                  &provisioning_client, &response, query_span, &query_span)))
      {
        printf("Failed to get operation status topic, return code %d\n", rc);
        return rc;
      }

      if (az_failed(rc = null_terminate_string(query_span)))
      {
        printf("Failed to get operation status topic, return code %d\n", rc);
        return rc;
      }

      printf("Querying after %u seconds...\n", response.retry_after_seconds);
      sleep_seconds(response.retry_after_seconds);

      if ((rc = MQTTClient_publish(mqtt_client, query_topic, 0, NULL, 0, 0, NULL))
          != MQTTCLIENT_SUCCESS)
      {
        printf("Failed to publish get_operation_status, return code %d\n", rc);
        return rc;
      }
    }
    else if (az_span_is_content_equal(response.registration_state, AZ_SPAN_FROM_STR("assigned")))
    {
      printf("SUCCESS - Device provisioned:\n");
      printf("\tHub Hostname: ");
      print_buffer(response.registration_information.assigned_hub_hostname);
      printf("\tDevice Id: ");
      print_buffer(response.registration_information.device_id);
      printf(
          "\tJSON Payload: (%d)\n", az_span_length(response.registration_information.json_payload));
      print_buffer(response.registration_information.json_payload);
      registration_complete = true;
    }
    else // unassigned, failed or disabled states
    {
      printf("ERROR - Device Provisioning failed:\n");
      printf("\tRegistration state: ");
      print_buffer(response.registration_state);
      printf("\tLast operation status: %d\n", response.status);
      printf("\tOperation ID: ");
      print_buffer(response.operation_id);
      printf("\tError code: %u\n", response.registration_information.extended_error_code);
      printf("\tError message: ");
      print_buffer(response.registration_information.error_message);
      printf("\tError timestamp: ");
      print_buffer(response.registration_information.error_timestamp);
      printf("\tError tracking ID: ");
      print_buffer(response.registration_information.error_tracking_id);
      if (response.retry_after_seconds > 0)
      {
        printf("\tRetry-after: %u seconds.", response.retry_after_seconds);
      }

      registration_complete = true;
    }

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
  }

  return 0;
}

int main()
{
  int rc;

  if ((rc = read_configuration_and_init_client()) != AZ_OK)
  {
    printf("Failed to read configuration from environment variables, return code %d\n", rc);
    return rc;
  }

  char client_id[128];
  az_span client_id_span = AZ_SPAN_LITERAL_FROM_BUFFER(client_id);
  if ((rc
       = az_iot_provisioning_client_id_get(&provisioning_client, client_id_span, &client_id_span))
      != AZ_OK)

  {
    printf("Failed to get MQTT clientId, return code %d\n", rc);
    return rc;
  }

  if ((rc = null_terminate_string(client_id_span)) != AZ_OK)
  {
    printf("Failed to get MQTT username, return code %d\n", rc);
    return rc;
  }

  if ((rc = MQTTClient_create(
           &mqtt_client,
           global_provisioning_endpoint,
           client_id,
           MQTTCLIENT_PERSISTENCE_NONE,
           NULL))
      != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to create MQTT client, return code %d\n", rc);
    return rc;
  }

  if ((rc = connect_device
()) != 0)
  {
    return rc;
  }

  printf("Connected to %s.\n", global_provisioning_endpoint);

  if ((rc = subscribe()) != 0)
  {
    return rc;
  }

  printf("Subscribed.\n");

  if ((rc = register_device()) != 0)
  {
    return rc;
  }

  if ((rc = get_operation_status()) != 0)
  {
    return rc;
  }

  if ((rc = MQTTClient_disconnect(mqtt_client, 10000)) != MQTTCLIENT_SUCCESS)
  {
    printf("Failed to disconnect MQTT client, return code %d\n", rc);
    return rc;
  }

  printf("Disconnected.\n");
  MQTTClient_destroy(&mqtt_client);

  return 0;
}
