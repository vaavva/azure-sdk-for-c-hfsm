// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_span.h>
#include <time.h>

// DO NOT MODIFY: Service information
#define ENV_GLOBAL_PROVISIONING_ENDPOINT_DEFAULT "ssl://global.azure-devices-provisioning.net:8883"
#define ENV_GLOBAL_PROVISIONING_ENDPOINT "AZ_IOT_GLOBAL_PROVISIONING_ENDPOINT"
#define ENV_ID_SCOPE_ENV "AZ_IOT_ID_SCOPE"

// DO NOT MODIFY: Device information
#define ENV_REGISTRATION_ID_ENV "AZ_IOT_REGISTRATION_ID"

// DO NOT MODIFY: the path to a PEM file containing the device certificate and
// key as well as any intermediate certificates chaining to an uploaded group certificate.
#define ENV_DEVICE_X509_CERT_PEM_FILE_PATH "AZ_IOT_DEVICE_X509_CERT_PEM_FILE"

// DO NOT MODIFY: the path to a PEM file containing the server trusted CA
// This is usually not needed on Linux or Mac but needs to be set on Windows.
#define ENV_DEVICE_X509_TRUST_PEM_FILE_PATH "AZ_IOT_DEVICE_X509_TRUST_PEM_FILE"

// Logging with formatting
#define LOG_ERROR(...) \
  { \
    (void)fprintf(stderr, "[%ld] ERROR:\t\t%s:%s():%d: ", (long)time(NULL), __FILE__, __func__, __LINE__); \
    (void)fprintf(stderr, __VA_ARGS__); \
    (void)fprintf(stderr, "\n"); \
    fflush(stdout); \
    fflush(stderr); \
  }
#define LOG_SUCCESS(...) \
  { \
    (void)printf("[%ld] SUCCESS:\t", (long)time(NULL)); \
    (void)printf(__VA_ARGS__); \
    (void)printf("\n"); \
  }
#define LOG(...) \
  { \
    (void)printf("\t\t"); \
    (void)printf(__VA_ARGS__); \
    (void)printf("\n"); \
  }
#define LOG_AZ_SPAN(span_description, span) \
  { \
    (void)printf("\t\t%s ", span_description); \
    char* buffer = (char*)az_span_ptr(span); \
    for (int32_t i = 0; i < az_span_size(span); i++) \
    { \
      putchar(*buffer++); \
    } \
    (void)printf("\n"); \
  }

// Buffers
static char x509_cert_pem_file_path_buffer[256];
static char x509_trust_pem_file_path_buffer[256];
static char global_provisioning_endpoint_buffer[256];
static char id_scope_buffer[16];
static char registration_id_buffer[256];
static char mqtt_client_id_buffer[128];
static char mqtt_username_buffer[128];
static char register_publish_topic_buffer[128];
static char query_topic_buffer[256];

#ifdef _MSC_VER
// "'getenv': This function or variable may be unsafe. Consider using _dupenv_s instead."
#pragma warning(disable : 4996)
#endif

AZ_INLINE az_result read_configuration_entry(
    const char* env_name,
    char* default_value,
    bool hide_value,
    az_span buffer,
    az_span* out_value)
{
  char* env_value = getenv(env_name);

  if (env_value == NULL && default_value != NULL)
  {
    env_value = default_value;
  }

  if (env_value != NULL)
  {
    (void)printf("%s = %s\n", env_name, hide_value ? "***" : env_value);
    az_span env_span = az_span_from_str(env_value);
    AZ_RETURN_IF_NOT_ENOUGH_SIZE(buffer, az_span_size(env_span));
    az_span_copy(buffer, env_span);
    *out_value = az_span_slice(buffer, 0, az_span_size(env_span));
  }
  else
  {
    LOG_ERROR("(missing) Please set the %s environment variable.", env_name);
    return AZ_ERROR_ARG;
  }

  return AZ_OK;
}

AZ_INLINE az_result read_environment_variables(
    az_span* endpoint,
    az_span* id_scope,
    az_span* registration_id)
{
  // Certification variables
  az_span device_cert = AZ_SPAN_FROM_BUFFER(x509_cert_pem_file_path_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_DEVICE_X509_CERT_PEM_FILE_PATH, NULL, false, device_cert, &device_cert));

  az_span trusted_cert = AZ_SPAN_FROM_BUFFER(x509_trust_pem_file_path_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_DEVICE_X509_TRUST_PEM_FILE_PATH, "", false, trusted_cert, &trusted_cert));

  // Connection variables
  *endpoint = AZ_SPAN_FROM_BUFFER(global_provisioning_endpoint_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_GLOBAL_PROVISIONING_ENDPOINT,
      ENV_GLOBAL_PROVISIONING_ENDPOINT_DEFAULT,
      false,
      *endpoint,
      endpoint));

  *id_scope = AZ_SPAN_FROM_BUFFER(id_scope_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(ENV_ID_SCOPE_ENV, NULL, false, *id_scope, id_scope));

  *registration_id = AZ_SPAN_FROM_BUFFER(registration_id_buffer);
  AZ_RETURN_IF_FAILED(read_configuration_entry(
      ENV_REGISTRATION_ID_ENV, NULL, false, *registration_id, registration_id));

  LOG(" "); // Log formatting
  return AZ_OK;
}
