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
#define TEST_PAYLOAD_RESERVE_SIZE 192

#define TEST_REGISTRATION_ID "myRegistrationId"
#define TEST_CUSTOM_PAYLOAD "{\"testData\":1, \"testData2\":{\"a\":\"b\"}}"
#define TEST_ID_SCOPE "0neFEEDC0DE"
#define TEST_OPERATION_ID "4.d0a671905ea5b2c8.42d78160-4c78-479e-8be7-61d5e55dac0d"


// CPOP_TODO: Integrate TrustBundle jsons.
#if 0
        // Response single cert - OK
        static const char* TEST_TRUST_BUNDLE_REGISTER_RESPONSE_SINGLE_CERT = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"payload\": {}," \
        "        \"trustBundle\": {" \
        "            \"certificates\": [" \
        "                {" \
        "                    \"certificate\": \"-----BEGIN CERTIFICATE-----\\r\\nMIIB6zCCAZKgAwIBAgIIZiChmKQYrUUwCgYIKoZIzj0EAwIwcDEyMDAGA1UEAwwpcm9vdC02YTQ2\\r\\nOWMxYS04MzlmLTQ2NjgtOGU4Yy04NjhlOGI0MGRkNGMxETAPBgNVBAsMCG1hcm9oZXJhMRcwFQYD\\r\\nVQQKDA5tYXJvaGVyYWxvYW5lcjEOMAwGA1UEBhMFMTYxNDgwHhcNMjEwOTA1MDcxOTQ4WhcNMjEw\\r\\nOTExMDcxOTQ4WjBwMTIwMAYDVQQDDClyb290LTZhNDY5YzFhLTgzOWYtNDY2OC04ZThjLTg2OGU4\\r\\nYjQwZGQ0YzERMA8GA1UECwwIbWFyb2hlcmExFzAVBgNVBAoMDm1hcm9oZXJhbG9hbmVyMQ4wDAYD\\r\\nVQQGEwUxNjE0ODBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOPnvxg7TfTGkfIDyJZgdkAWrhZ4\\r\\nkNCzZoBdujF9BdIe9yQKrM3sskfKBMJTAp0Mx7Hr6PM4qU0UNxispFW6uRyjFjAUMBIGA1UdEwEB\\r\\n/wQIMAYBAf8CAQEwCgYIKoZIzj0EAwIDRwAwRAIgFlF0vGuAIWOVEhZ0jYmA4IO0agbFbq1KOTfN\\r\\nq4AVzwgCIAgxApsBGYwslDboPToMR8zTkG8mTdd3I/MNf8qQBKfm\\r\\n-----END CERTIFICATE-----\\r\\n\"," \
        "                    \"metadata\": {" \
        "                        \"subjectName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"sha1Thumbprint\": \"5C57EB7EED8E278B1EAE278AAE1C6083EC2861B0\"," \
        "                        \"sha256Thumbprint\": \"1EA3870D991E432B5895FD9C8845D859AFDA933569B7FA5855050CC1C2DFD1E5\"," \
        "                        \"issuerName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"notBeforeUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"notAfterUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"serialNumber\": \"6620A198A418AD45\"," \
        "                        \"version\": 3" \
        "                    }" \
        "                }" \
        "            ]," \
        "            \"id\": \"TestTrustBundle-4e697b1f-1b59-4587-bdc7-e8645eef0a13\"," \
        "            \"createdDateTime\": \"2019-08-24T14:15:22Z\"," \
        "            \"lastModifiedDateTime\": \"2019-08-24T14:15:22Z\"," \
        "             \"etag\": \"\\\"5301e830-0000-0300-0000-613864190000\\\"\"" \
        "        }" \
        "    }" \
        "}";

        // Response NULL = OK
        static const char* TEST_TRUST_BUNDLE_REGISTER_RESPONSE_NULL = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"payload\": {}," \
        "        \"trustBundle\": null" \
        "    }" \
        "}";


        // Multiple certs - OK
        static const char* TEST_TRUST_BUNDLE_REGISTER_RESPONSE_MULTIPLE_CERTS = 
        "{" \
        "    \"operationId\": \"string\"," \
        "    \"status\": \"assigned\"," \
        "    \"registrationState\": {" \
        "        \"tpm\": {" \
        "            \"authenticationKey\": \"dGVzdF9hdXRoX2tleQ==\"" \
        "        }," \
        "        \"registrationId\": \"string\"," \
        "        \"createdDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"assignedHub\": \"string\"," \
        "        \"deviceId\": \"string\"," \
        "        \"status\": \"assigned\"," \
        "        \"substatus\": \"initialAssignment\"," \
        "        \"errorCode\": -2147483648," \
        "        \"errorMessage\": \"string\"," \
        "        \"lastUpdatedDateTimeUtc\": \"2019-08-24T14:15:22Z\"," \
        "        \"etag\": \"string\"," \
        "        \"payload\": {}," \
        "        \"trustBundle\": {" \
        "            \"certificates\": [" \
        "                {" \
        "                    \"certificate\": \"-----BEGIN CERTIFICATE-----\\r\\nMIIB6zCCAZKgAwIBAgIIZiChmKQYrUUwCgYIKoZIzj0EAwIwcDEyMDAGA1UEAwwpcm9vdC02YTQ2\\r\\nOWMxYS04MzlmLTQ2NjgtOGU4Yy04NjhlOGI0MGRkNGMxETAPBgNVBAsMCG1hcm9oZXJhMRcwFQYD\\r\\nVQQKDA5tYXJvaGVyYWxvYW5lcjEOMAwGA1UEBhMFMTYxNDgwHhcNMjEwOTA1MDcxOTQ4WhcNMjEw\\r\\nOTExMDcxOTQ4WjBwMTIwMAYDVQQDDClyb290LTZhNDY5YzFhLTgzOWYtNDY2OC04ZThjLTg2OGU4\\r\\nYjQwZGQ0YzERMA8GA1UECwwIbWFyb2hlcmExFzAVBgNVBAoMDm1hcm9oZXJhbG9hbmVyMQ4wDAYD\\r\\nVQQGEwUxNjE0ODBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOPnvxg7TfTGkfIDyJZgdkAWrhZ4\\r\\nkNCzZoBdujF9BdIe9yQKrM3sskfKBMJTAp0Mx7Hr6PM4qU0UNxispFW6uRyjFjAUMBIGA1UdEwEB\\r\\n/wQIMAYBAf8CAQEwCgYIKoZIzj0EAwIDRwAwRAIgFlF0vGuAIWOVEhZ0jYmA4IO0agbFbq1KOTfN\\r\\nq4AVzwgCIAgxApsBGYwslDboPToMR8zTkG8mTdd3I/MNf8qQBKfm\\r\\n-----END CERTIFICATE-----\\r\\n\"," \
        "                    \"metadata\": {" \
        "                        \"subjectName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"sha1Thumbprint\": \"5C57EB7EED8E278B1EAE278AAE1C6083EC2861B0\"," \
        "                        \"sha256Thumbprint\": \"1EA3870D991E432B5895FD9C8845D859AFDA933569B7FA5855050CC1C2DFD1E5\"," \
        "                        \"issuerName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"notBeforeUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"notAfterUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"serialNumber\": \"6620A198A418AD45\"," \
        "                        \"version\": 3" \
        "                    }" \
        "                }," \
        "                {" \
        "                    \"certificate\": \"-----BEGIN CERTIFICATE-----\\r\\nMIIB6zCCAZKgAwIBAgIIZiChmKQYrUUwCgYIKoZIzj0EAwIwcDEyMDAGA1UEAwwpcm9vdC02YTQ2\\r\\nOWMxYS04MzlmLTQ2NjgtOGU4Yy04NjhlOGI0MGRkNGMxETAPBgNVBAsMCG1hcm9oZXJhMRcwFQYD\\r\\nVQQKDA5tYXJvaGVyYWxvYW5lcjEOMAwGA1UEBhMFMTYxNDgwHhcNMjEwOTA1MDcxOTQ4WhcNMjEw\\r\\nOTExMDcxOTQ4WjBwMTIwMAYDVQQDDClyb290LTZhNDY5YzFhLTgzOWYtNDY2OC04ZThjLTg2OGU4\\r\\nYjQwZGQ0YzERMA8GA1UECwwIbWFyb2hlcmExFzAVBgNVBAoMDm1hcm9oZXJhbG9hbmVyMQ4wDAYD\\r\\nVQQGEwUxNjE0ODBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOPnvxg7TfTGkfIDyJZgdkAWrhZ4\\r\\nkNCzZoBdujF9BdIe9yQKrM3sskfKBMJTAp0Mx7Hr6PM4qU0UNxispFW6uRyjFjAUMBIGA1UdEwEB\\r\\n/wQIMAYBAf8CAQEwCgYIKoZIzj0EAwIDRwAwRAIgFlF0vGuAIWOVEhZ0jYmA4IO0agbFbq1KOTfN\\r\\nq4AVzwgCIAgxApsBGYwslDboPToMR8zTkG8mTdd3I/MNf8qQBKfm\\r\\n-----END CERTIFICATE-----\\r\\n\"," \
        "                    \"metadata\": {" \
        "                        \"subjectName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"sha1Thumbprint\": \"5C57EB7EED8E278B1EAE278AAE1C6083EC2861B0\"," \
        "                        \"sha256Thumbprint\": \"1EA3870D991E432B5895FD9C8845D859AFDA933569B7FA5855050CC1C2DFD1E5\"," \
        "                        \"issuerName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"notBeforeUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"notAfterUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"serialNumber\": \"6620A198A418AD45\"," \
        "                        \"version\": 3" \
        "                    }" \
        "                }," \
        "                {" \
        "                    \"certificate\": \"-----BEGIN CERTIFICATE-----\\r\\nMIIB6zCCAZKgAwIBAgIIZiChmKQYrUUwCgYIKoZIzj0EAwIwcDEyMDAGA1UEAwwpcm9vdC02YTQ2\\r\\nOWMxYS04MzlmLTQ2NjgtOGU4Yy04NjhlOGI0MGRkNGMxETAPBgNVBAsMCG1hcm9oZXJhMRcwFQYD\\r\\nVQQKDA5tYXJvaGVyYWxvYW5lcjEOMAwGA1UEBhMFMTYxNDgwHhcNMjEwOTA1MDcxOTQ4WhcNMjEw\\r\\nOTExMDcxOTQ4WjBwMTIwMAYDVQQDDClyb290LTZhNDY5YzFhLTgzOWYtNDY2OC04ZThjLTg2OGU4\\r\\nYjQwZGQ0YzERMA8GA1UECwwIbWFyb2hlcmExFzAVBgNVBAoMDm1hcm9oZXJhbG9hbmVyMQ4wDAYD\\r\\nVQQGEwUxNjE0ODBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABOPnvxg7TfTGkfIDyJZgdkAWrhZ4\\r\\nkNCzZoBdujF9BdIe9yQKrM3sskfKBMJTAp0Mx7Hr6PM4qU0UNxispFW6uRyjFjAUMBIGA1UdEwEB\\r\\n/wQIMAYBAf8CAQEwCgYIKoZIzj0EAwIDRwAwRAIgFlF0vGuAIWOVEhZ0jYmA4IO0agbFbq1KOTfN\\r\\nq4AVzwgCIAgxApsBGYwslDboPToMR8zTkG8mTdd3I/MNf8qQBKfm\\r\\n-----END CERTIFICATE-----\\r\\n\"," \
        "                    \"metadata\": {" \
        "                        \"subjectName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"sha1Thumbprint\": \"5C57EB7EED8E278B1EAE278AAE1C6083EC2861B0\"," \
        "                        \"sha256Thumbprint\": \"1EA3870D991E432B5895FD9C8845D859AFDA933569B7FA5855050CC1C2DFD1E5\"," \
        "                        \"issuerName\": \"CN=root-6a469c1a-839f-4668-8e8c-868e8b40dd4c, OU=test, O=testloaner, C=16148\"," \
        "                        \"notBeforeUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"notAfterUtc\": \"2019-08-24T14:15:22Z\"," \
        "                        \"serialNumber\": \"6620A198A418AD45\"," \
        "                        \"version\": 3" \
        "                    }" \
        "                }" \
        "            ]," \
        "            \"id\": \"TestTrustBundle-4e697b1f-1b59-4587-bdc7-e8645eef0a13\"," \
        "            \"createdDateTime\": \"2019-08-24T14:15:22Z\"," \
        "            \"lastModifiedDateTime\": \"2019-08-24T14:15:22Z\"," \
        "             \"etag\": \"\\\"5301e830-0000-0300-0000-613864190000\\\"\"" \
        "        }" \
        "    }" \
        "}";        
#endif


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

  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      NULL, AZ_SPAN_EMPTY, NULL, payload, sizeof(payload), &payload_len));
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

  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      &client,
      AZ_SPAN_EMPTY,
      (az_iot_provisioning_client_payload_options*)payload,
      payload,
      sizeof(payload),
      &payload_len));
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

  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      &client, AZ_SPAN_EMPTY, NULL, NULL, 1, &payload_len));
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

  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      &client, AZ_SPAN_EMPTY, NULL, payload, 0, &payload_len));
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

  ASSERT_PRECONDITION_CHECKED(az_iot_provisioning_client_get_request_payload(
      &client, AZ_SPAN_EMPTY, NULL, payload, 1, NULL));
}

#endif

static void test_az_iot_provisioning_client_get_request_payload_no_custom_payload()
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

  ret = az_iot_provisioning_client_get_request_payload(
      &client, AZ_SPAN_EMPTY, NULL, payload, sizeof(payload), &payload_len);
  assert_int_equal(AZ_OK, ret);
  assert_int_equal(payload_len, expected_payload_len);
  assert_memory_equal(expected_payload, payload, expected_payload_len);
  assert_int_equal((uint8_t)0xCC, payload[expected_payload_len]);
}

static void test_az_iot_provisioning_client_get_request_payload_custom_payload()
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

  ret = az_iot_provisioning_client_get_request_payload(
      &client, test_custom_payload, NULL, payload, sizeof(payload), &payload_len);
  assert_int_equal(AZ_OK, ret);
  assert_int_equal(payload_len, expected_payload_len);
  assert_memory_equal(expected_payload, payload, expected_payload_len);
  assert_int_equal((uint8_t)0xCC, payload[expected_payload_len]);
}

int test_az_iot_provisioning_client_payload()
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

    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_no_custom_payload),
    cmocka_unit_test(test_az_iot_provisioning_client_get_request_payload_custom_payload),
  };

  return cmocka_run_group_tests_name("az_iot_provisioning_client_payload", tests, NULL, NULL);
}
