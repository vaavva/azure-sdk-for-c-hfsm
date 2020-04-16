// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "test_az_iot_provisioning_client.h"
#include <az_iot_provisioning_client.h>
#include <az_span.h>
#include <az_test_span.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

// TODO: #564 - Remove the use of the _az_cfh.h header in samples.
#include <_az_cfg.h>

#define TEST_REGISTRATION_ID "myRegistrationId"
#define TEST_HUB_HOSTNAME "contoso.azure-devices.net"
#define TEST_DEVICE_ID "my-device-id1"
#define TEST_OPERATION_ID "4.d0a671905ea5b2c8.42d78160-4c78-479e-8be7-61d5e55dac0d"

#define TEST_STATUS_ASSIGNING "assigning"
#define TEST_STATUS_ASSIGNED "assigned"
#define TEST_STATUS_FAILED "failed"
#define TEST_STATUS_DISABLED "disabled"

#define TEST_ERROR_MESSAGE "Invalid certificate."
#define TEST_ERROR_TRACKING_ID "8ad0463c-6427-4479-9dfa-3e8bb7003e9b"
#define TEST_ERROR_TIMESTAMP "2020-04-10T05:24:22.4718526Z"

static void test_az_iot_provisioning_client_received_topic_payload_parse_assigning_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/202/?$rid=1&retry-after=3");
  az_span received_payload = AZ_SPAN_FROM_STR("{\"operationId\":\"" TEST_OPERATION_ID
                                              "\",\"status\":\"" TEST_STATUS_ASSIGNING "\"}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_received_topic_payload_parse(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_ACCEPTED, response.status); // 202
  assert_int_equal(3, response.retry_after_seconds);

  // From payload
  az_span_for_test_verify(
      response.operation_id,
      TEST_OPERATION_ID,
      (uint32_t)strlen(TEST_OPERATION_ID),
      (uint32_t)strlen(TEST_OPERATION_ID));

  az_span_for_test_verify(
      response.registration_state,
      TEST_STATUS_ASSIGNING,
      (uint32_t)strlen(TEST_STATUS_ASSIGNING),
      (uint32_t)strlen(TEST_STATUS_ASSIGNING));
}

static void test_az_iot_provisioning_client_received_topic_payload_parse_assigning2_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/202/?retry-after=120&$rid=1");
  az_span received_payload = AZ_SPAN_FROM_STR(
      "{\"operationId\":\"" TEST_OPERATION_ID "\",\"status\":\"" TEST_STATUS_ASSIGNING
      "\",\"registrationState\":{\"registrationId\":\"" TEST_REGISTRATION_ID
      "\",\"status\":\"" TEST_STATUS_ASSIGNING "\"}}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_received_topic_payload_parse(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_ACCEPTED, response.status); // 202
  assert_int_equal(120, response.retry_after_seconds);

  // From payload
  az_span_for_test_verify(
      response.operation_id,
      TEST_OPERATION_ID,
      (uint32_t)strlen(TEST_OPERATION_ID),
      (uint32_t)strlen(TEST_OPERATION_ID));

  az_span_for_test_verify(
      response.registration_state,
      TEST_STATUS_ASSIGNING,
      (uint32_t)strlen(TEST_STATUS_ASSIGNING),
      (uint32_t)strlen(TEST_STATUS_ASSIGNING));
}

static void test_az_iot_provisioning_client_received_topic_payload_parse_assigned_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload
      = AZ_SPAN_FROM_STR("{\"operationId\":\"" TEST_OPERATION_ID
                         "\",\"status\":\"" TEST_STATUS_ASSIGNED "\",\"registrationState\":{"
                         "\"x509\":{},"
                         "\"registrationId\":\"" TEST_REGISTRATION_ID "\","
                         "\"createdDateTimeUtc\":\"2020-04-10T03:11:13.0276997Z\","
                         "\"assignedHub\":\"contoso.azure-devices.net\","
                         "\"deviceId\":\"" TEST_DEVICE_ID "\","
                         "\"status\":\"" TEST_STATUS_ASSIGNED "\","
                         "\"substatus\":\"initialAssignment\","
                         "\"lastUpdatedDateTimeUtc\":\"2020-04-10T03:11:13.2096201Z\","
                         "\"payload\": {\"myArray\": [1, 2, 3, 4, 5],"
                         "\"myCustomAllocationProperty\":\"123\"},"
                         "\"etag\":\"IjYxMDA4ZDQ2LTAwMDAtMDEwMC0wMDAwLTVlOGZlM2QxMDAwMCI=\"}}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_received_topic_payload_parse(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_OK, response.status); // 200
  assert_int_equal(0, response.retry_after_seconds);

  // From payload
  az_span_for_test_verify(
      response.operation_id,
      TEST_OPERATION_ID,
      (uint32_t)strlen(TEST_OPERATION_ID),
      (uint32_t)strlen(TEST_OPERATION_ID));

  az_span_for_test_verify(
      response.registration_state,
      TEST_STATUS_ASSIGNED,
      (uint32_t)strlen(TEST_STATUS_ASSIGNED),
      (uint32_t)strlen(TEST_STATUS_ASSIGNED));

  az_span_for_test_verify(
      response.registration_information.assigned_hub_hostname,
      TEST_HUB_HOSTNAME,
      (uint32_t)strlen(TEST_HUB_HOSTNAME),
      (uint32_t)strlen(TEST_HUB_HOSTNAME));

  az_span_for_test_verify(
      response.registration_information.device_id,
      TEST_DEVICE_ID,
      (uint32_t)strlen(TEST_DEVICE_ID),
      (uint32_t)strlen(TEST_DEVICE_ID));

  assert_int_equal(200, response.registration_information.status);
  assert_int_equal(0, response.registration_information.extended_error_code);
  assert_true(0 == az_span_length(response.registration_information.error_message));
}

static void test_az_iot_provisioning_client_received_topic_payload_parse_error_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/401/?$rid=1");
  az_span received_payload = AZ_SPAN_FROM_STR(
      "{\"errorCode\":401002,\"trackingId\":\"" TEST_ERROR_TRACKING_ID "\","
      "\"message\":\"" TEST_ERROR_MESSAGE "\",\"timestampUtc\":\"" TEST_ERROR_TIMESTAMP "\"}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_received_topic_payload_parse(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_UNAUTHORIZED, response.status); // 401
  assert_int_equal(0, response.retry_after_seconds);

  // From payload
  assert_true(0 == az_span_length(response.operation_id));

  az_span_for_test_verify(
      response.registration_state,
      TEST_STATUS_FAILED,
      (uint32_t)strlen(TEST_STATUS_FAILED),
      (uint32_t)strlen(TEST_STATUS_FAILED));

  assert_true(0 == az_span_length(response.registration_information.assigned_hub_hostname));
  assert_true(0 == az_span_length(response.registration_information.device_id));

  assert_int_equal(AZ_IOT_STATUS_UNAUTHORIZED, response.registration_information.status);
  assert_int_equal(401002, response.registration_information.extended_error_code);

  az_span_for_test_verify(
      response.registration_information.error_message,
      TEST_ERROR_MESSAGE,
      (uint32_t)strlen(TEST_ERROR_MESSAGE),
      (uint32_t)strlen(TEST_ERROR_MESSAGE));

  az_span_for_test_verify(
      response.registration_information.error_timestamp,
      TEST_ERROR_TIMESTAMP,
      (uint32_t)strlen(TEST_ERROR_TIMESTAMP),
      (uint32_t)strlen(TEST_ERROR_TIMESTAMP));

  az_span_for_test_verify(
      response.registration_information.error_tracking_id,
      TEST_ERROR_TRACKING_ID,
      (uint32_t)strlen(TEST_ERROR_TRACKING_ID),
      (uint32_t)strlen(TEST_ERROR_TRACKING_ID));
}

static void test_az_iot_provisioning_client_received_topic_payload_parse_disabled_state_succeed()
{
  az_iot_provisioning_client client;
  az_span received_topic = AZ_SPAN_FROM_STR("$dps/registrations/res/200/?$rid=1");
  az_span received_payload = AZ_SPAN_FROM_STR(
      "{\"operationId\":\"\",\"status\":\"" TEST_STATUS_DISABLED
      "\",\"registrationState\":{\"registrationId\":\"" TEST_REGISTRATION_ID
      "\",\"status\":\"" TEST_STATUS_DISABLED "\"}}");

  az_iot_provisioning_client_register_response response;
  az_result ret = az_iot_provisioning_client_received_topic_payload_parse(
      &client, received_topic, received_payload, &response);
  assert_int_equal(AZ_OK, ret);

  // From topic
  assert_int_equal(AZ_IOT_STATUS_OK, response.status); // 200
  assert_int_equal(0, response.retry_after_seconds);

  // From payload
  assert_int_equal(0, az_span_length(response.operation_id));
  az_span_for_test_verify(
      response.registration_state,
      TEST_STATUS_DISABLED,
      (uint32_t)strlen(TEST_STATUS_DISABLED),
      (uint32_t)strlen(TEST_STATUS_DISABLED));
}

#ifdef _MSC_VER
// warning C4113: 'void (__cdecl *)()' differs in parameter lists from 'CMUnitTestFunction'
#pragma warning(disable : 4113)
#endif

int test_az_iot_provisioning_client_parser()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_payload_parse_assigning_state_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_payload_parse_assigning2_state_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_payload_parse_assigned_state_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_payload_parse_error_state_succeed),
    cmocka_unit_test(
        test_az_iot_provisioning_client_received_topic_payload_parse_disabled_state_succeed),
  };

  return cmocka_run_group_tests_name("az_iot_provisioning_client_parser", tests, NULL, NULL);
}
