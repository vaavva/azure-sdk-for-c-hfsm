// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_provisioning_client.h"
#include <az_test_span.h>
#include <azure/core/az_precondition.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/az_iot_provisioning_client.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <az_test_precondition.h>
#include <cmocka.h>

#include <azure/core/_az_cfg.h>

// Unlike an az_span, the az_json_writer requires additional reserve buffer than
// the absolute minimum number of bytes.  (See _az_MINIMUM_STRING_CHUNK_SIZE e.g.)
// Since we use az_json_writer in the implementation, we need to reserve additional bytes
// for tests.
#define TEST_PAYLOAD_RESERVE_SIZE 1024

#define TEST_REGISTRATION_ID "myRegistrationId"
#define TEST_CUSTOM_PAYLOAD "{\"testData\":1, \"testData2\":{\"a\":\"b\"}}"
#define TEST_ID_SCOPE "0neFEEDC0DE"
#define TEST_OPERATION_ID "4.d0a671905ea5b2c8.42d78160-4c78-479e-8be7-61d5e55dac0d"
#define TEST_CSR                                                       \
  "-----BEGIN CERTIFICATE REQUEST-----\n"                              \
  "MIHxMIGYAgEAMDYxNDAyBgNVBAMMK2RldmljZS1hOTBkNDMzYi02ZDUyLTRkNzMt\n" \
  "OGY5OS1lMWM3OWQ4Zjg0NDQwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATPa5lj\n" \
  "HzjjUtUvke8vZ0ATXujUoYpPPlUQKXaZu5qxyP7iCwJNal6z5niXy8ZDqmbO6coh\n" \
  "DTFeRqW/WcvFzu3RoAAwCgYIKoZIzj0EAwIDSAAwRQIhAPe+tJNc4qDpoc+p/38J\n" \
  "YjOpoGmAU/Fs7Xa77Nq9cEkQAiBhDVtxbAPk6xDkFZRTMY1eMnIvLFDXw+rbJcSG\n" \
  "D4jwfg==\n"                                                         \
  "-----END CERTIFICATE REQUEST-----\n"

#define TEST_CSR_ESCAPED                                                \
  "-----BEGIN CERTIFICATE REQUEST-----\\n"                              \
  "MIHxMIGYAgEAMDYxNDAyBgNVBAMMK2RldmljZS1hOTBkNDMzYi02ZDUyLTRkNzMt\\n" \
  "OGY5OS1lMWM3OWQ4Zjg0NDQwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAATPa5lj\\n" \
  "HzjjUtUvke8vZ0ATXujUoYpPPlUQKXaZu5qxyP7iCwJNal6z5niXy8ZDqmbO6coh\\n" \
  "DTFeRqW/WcvFzu3RoAAwCgYIKoZIzj0EAwIDSAAwRQIhAPe+tJNc4qDpoc+p/38J\\n" \
  "YjOpoGmAU/Fs7Xa77Nq9cEkQAiBhDVtxbAPk6xDkFZRTMY1eMnIvLFDXw+rbJcSG\\n" \
  "D4jwfg==\\n"                                                         \
  "-----END CERTIFICATE REQUEST-----\\n"

static const az_span test_global_device_hostname
    = AZ_SPAN_LITERAL_FROM_STR("global.azure-devices-provisioning.net");

static const az_span test_custom_payload = AZ_SPAN_LITERAL_FROM_STR(TEST_CUSTOM_PAYLOAD);

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

#ifndef AZ_NO_PRECONDITION_CHECKING
ENABLE_PRECONDITION_CHECK_TESTS()

static void test_az_iot_provisioning_client_get_request_payload_NULL_client_fails()
{
  uint8_t payload[TEST_PAYLOAD_RESERVE_SIZE];
  size_t payload_len;

  AZ_PUSH_IGNORE_DEPRECATIONS 
  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      NULL, AZ_SPAN_EMPTY, NULL, payload, sizeof(payload), &payload_len));
  AZ_POP_WARNINGS
}

static void test_az_iot_provisioning_client_get_request_payload_non_NULL_reserved_fails()
{
  az_iot_provisioning_client client;
  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_hostname,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      NULL);
  assert_int_equal(AZ_OK, ret);

  uint8_t payload[TEST_PAYLOAD_RESERVE_SIZE];
  size_t payload_len;

  AZ_PUSH_IGNORE_DEPRECATIONS 
  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      &client,
      AZ_SPAN_EMPTY,
      (az_iot_provisioning_client_payload_options*)payload,
      payload,
      sizeof(payload),
      &payload_len));
  AZ_POP_WARNINGS
}

static void test_az_iot_provisioning_client_get_request_payload_NULL_mqtt_payload_fails()
{
  az_iot_provisioning_client client;
  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_hostname,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      NULL);
  assert_int_equal(AZ_OK, ret);

  size_t payload_len;

  AZ_PUSH_IGNORE_DEPRECATIONS 
  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      &client, AZ_SPAN_EMPTY, NULL, NULL, 1, &payload_len));
  AZ_POP_WARNINGS
}

static void test_az_iot_provisioning_client_get_request_payload_zero_payload_size_fails()
{
  az_iot_provisioning_client client;
  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_hostname,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      NULL);
  assert_int_equal(AZ_OK, ret);

  uint8_t payload[TEST_PAYLOAD_RESERVE_SIZE];
  size_t payload_len;

  AZ_PUSH_IGNORE_DEPRECATIONS 
  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      &client, AZ_SPAN_EMPTY, NULL, payload, 0, &payload_len));
  AZ_POP_WARNINGS
}

static void test_az_iot_provisioning_client_get_request_payload_NULL_payload_length_fails()
{
  az_iot_provisioning_client client;
  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_hostname,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      NULL);
  assert_int_equal(AZ_OK, ret);

  uint8_t payload[TEST_PAYLOAD_RESERVE_SIZE];

  AZ_PUSH_IGNORE_DEPRECATIONS 
  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      &client, AZ_SPAN_EMPTY, NULL, payload, 1, NULL));
  AZ_POP_WARNINGS
}

#endif // AZ_NO_PRECONDITION_CHECKING

static void test_az_iot_provisioning_client_get_request_payload_no_custom_payload_succeed()
{
  az_iot_provisioning_client client = { 0 };
  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_hostname,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      NULL);
  assert_int_equal(AZ_OK, ret);

  char expected_payload[] = "{\"registrationId\":\"" TEST_REGISTRATION_ID "\"}";
  size_t expected_payload_len = sizeof(expected_payload) - 1;

  uint8_t payload[TEST_PAYLOAD_RESERVE_SIZE];
  memset(payload, 0xCC, sizeof(payload));
  size_t payload_len = 0xBAADC0DE;

  AZ_PUSH_IGNORE_DEPRECATIONS 
  ret = az_iot_provisioning_client_get_request_payload(
      &client, AZ_SPAN_EMPTY, NULL, payload, sizeof(payload), &payload_len);
  AZ_POP_WARNINGS
  assert_int_equal(AZ_OK, ret);
  assert_int_equal(payload_len, expected_payload_len);
  assert_memory_equal(expected_payload, payload, expected_payload_len);
  assert_int_equal((uint8_t)0xCC, payload[expected_payload_len]);
}

static void test_az_iot_provisioning_client_get_request_payload_custom_payload_succeed()
{
  az_iot_provisioning_client client = { 0 };
  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_hostname,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      NULL);
  assert_int_equal(AZ_OK, ret);

  char expected_payload[]
      = "{\"registrationId\":\"" TEST_REGISTRATION_ID "\",\"payload\":" TEST_CUSTOM_PAYLOAD "}";
  size_t expected_payload_len = sizeof(expected_payload) - 1;

  uint8_t payload[TEST_PAYLOAD_RESERVE_SIZE];
  memset(payload, 0xCC, sizeof(payload));
  size_t payload_len = 0xBAADC0DE;

  AZ_PUSH_IGNORE_DEPRECATIONS 
  ret = az_iot_provisioning_client_get_request_payload(
      &client, test_custom_payload, NULL, payload, sizeof(payload), &payload_len);
  assert_int_equal(AZ_OK, ret);
  assert_int_equal(payload_len, expected_payload_len);
  assert_memory_equal(expected_payload, payload, expected_payload_len);
  assert_int_equal((uint8_t)0xCC, payload[expected_payload_len]);
  AZ_POP_WARNINGS
}

static void test_az_iot_provisioning_client_register_get_request_payload_with_csr_succeed()
{
  az_iot_provisioning_client client = { 0 };
  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_hostname,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      NULL);
  assert_int_equal(AZ_OK, ret);

  char* test_csr = TEST_CSR;

  char expected_payload[]
      = "{\"registrationId\":\"" TEST_REGISTRATION_ID "\",\"payload\":" TEST_CUSTOM_PAYLOAD
        ",\"clientCertificateCsr\":\"" TEST_CSR_ESCAPED "\"}";
  size_t expected_payload_len = sizeof(expected_payload) - 1;

  uint8_t payload[TEST_PAYLOAD_RESERVE_SIZE];
  memset(payload, 0xCC, sizeof(payload));
  size_t payload_len = 0xBAADC0DE;

  az_iot_provisioning_client_payload_options options
      = az_iot_provisioning_client_payload_options_default();
  options.certificate_signing_request = az_span_create_from_str(test_csr);

  ret = az_iot_provisioning_client_register_get_request_payload(
      &client, test_custom_payload, &options, payload, sizeof(payload), &payload_len);

  assert_int_equal(AZ_OK, ret);
  assert_int_equal(payload_len, expected_payload_len);
  assert_memory_equal(expected_payload, payload, expected_payload_len);
  assert_int_equal((uint8_t)0xCC, payload[expected_payload_len]);
}

static void test_az_iot_provisioning_client_register_get_request_payload_NULL_options_succeed()
{
  az_iot_provisioning_client client = { 0 };
  az_result ret = az_iot_provisioning_client_init(
      &client,
      test_global_device_hostname,
      AZ_SPAN_FROM_STR(TEST_ID_SCOPE),
      AZ_SPAN_FROM_STR(TEST_REGISTRATION_ID),
      NULL);
  assert_int_equal(AZ_OK, ret);

  char expected_payload[]
      = "{\"registrationId\":\"" TEST_REGISTRATION_ID "\",\"payload\":" TEST_CUSTOM_PAYLOAD "}";
  size_t expected_payload_len = sizeof(expected_payload) - 1;

  uint8_t payload[TEST_PAYLOAD_RESERVE_SIZE];
  memset(payload, 0xCC, sizeof(payload));
  size_t payload_len = 0xBAADC0DE;

  ret = az_iot_provisioning_client_register_get_request_payload(
      &client, test_custom_payload, NULL, payload, sizeof(payload), &payload_len);

  assert_int_equal(AZ_OK, ret);
  assert_int_equal(payload_len, expected_payload_len);
  assert_memory_equal(expected_payload, payload, expected_payload_len);
  assert_int_equal((uint8_t)0xCC, payload[expected_payload_len]);
}

int test_az_iot_provisioning_client_register_get_request_payload()
{
#ifndef AZ_NO_PRECONDITION_CHECKING
  SETUP_PRECONDITION_CHECK_TESTS();
#endif // AZ_NO_PRECONDITION_CHECKING

  const struct CMUnitTest tests[] = {
#ifndef AZ_NO_PRECONDITION_CHECKING
    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_NULL_client_fails),
    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_non_NULL_reserved_fails),
    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_NULL_mqtt_payload_fails),
    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_zero_payload_size_fails),
    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_NULL_payload_length_fails),
#endif // AZ_NO_PRECONDITION_CHECKING

    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_no_custom_payload_succeed),
    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_custom_payload_succeed),
    cmocka_unit_test(test_az_iot_provisioning_client_register_get_request_payload_with_csr_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_register_get_request_payload_NULL_options_succeed),
  };

  return cmocka_run_group_tests_name("az_iot_provisioning_client_payload", tests, NULL, NULL);
}
