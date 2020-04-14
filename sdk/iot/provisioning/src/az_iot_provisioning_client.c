// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_iot_provisioning_client.h"
#include <az_json.h>
#include <az_precondition_internal.h>
#include <az_result.h>
#include <az_span.h>

#include <_az_cfg.h>

static const az_span provisioning_service_api_version
    = AZ_SPAN_LITERAL_FROM_STR("/api-version=" AZ_IOT_PROVISIONING_SERVICE_VERSION);

static const az_span str_dps_registrations_res
    = AZ_SPAN_LITERAL_FROM_STR("$dps/registrations/res/");
static const int32_t idx_registrations_start = 4;
static const int32_t idx_registrations_end = 19;

static const int32_t idx_res_start = 19;

static const az_span str_put_iotdps_register
    = AZ_SPAN_LITERAL_FROM_STR("PUT/iotdps-register/?$rid=1");
static const az_span str_get_iotdps_get_operationstatus
    = AZ_SPAN_LITERAL_FROM_STR("GET/iotdps-get-operationstatus/?$rid=1&operationId=");

AZ_NODISCARD az_iot_provisioning_client_options az_iot_provisioning_client_options_default()
{
  return (az_iot_provisioning_client_options){ .user_agent = AZ_SPAN_NULL };
}

AZ_NODISCARD az_result az_iot_provisioning_client_init(
    az_iot_provisioning_client* client,
    az_span global_device_endpoint,
    az_span id_scope,
    az_span registration_id,
    az_iot_provisioning_client_options const* options)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(global_device_endpoint, 1, false);
  AZ_PRECONDITION_VALID_SPAN(id_scope, 1, false);
  AZ_PRECONDITION_VALID_SPAN(registration_id, 1, false);

  client->_internal.global_device_endpoint = global_device_endpoint;
  client->_internal.id_scope = id_scope;
  client->_internal.registration_id = registration_id;

  client->_internal.options
      = options == NULL ? az_iot_provisioning_client_options_default() : *options;

  return AZ_OK;
}

// <id_scope>/registrations/<registration_id>/api-version=<service_version>
AZ_NODISCARD az_result az_iot_provisioning_client_user_name_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_user_name,
    az_span* out_mqtt_user_name)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_user_name, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_user_name);

  const az_span* const user_agent = &(client->_internal.options.user_agent);
  az_span str_registrations
      = az_span_slice(str_dps_registrations_res, idx_registrations_start, idx_registrations_end);

  int32_t required_length = az_span_length(client->_internal.id_scope)
      + az_span_length(str_registrations) + az_span_length(client->_internal.registration_id)
      + az_span_length(provisioning_service_api_version);
  if (az_span_length(*user_agent) > 0)
  {
    required_length += az_span_length(*user_agent) + 1;
  }

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(mqtt_user_name, required_length);

  mqtt_user_name = az_span_copy(mqtt_user_name, client->_internal.id_scope);
  mqtt_user_name = az_span_append(mqtt_user_name, str_registrations);
  mqtt_user_name = az_span_append(mqtt_user_name, client->_internal.registration_id);
  mqtt_user_name = az_span_append(mqtt_user_name, provisioning_service_api_version);

  if (az_span_length(*user_agent) > 0)
  {
    // TODO: check user_agent string proper format.
    mqtt_user_name = az_span_append_uint8(mqtt_user_name, '&');
    mqtt_user_name = az_span_append(mqtt_user_name, *user_agent);
  }

  *out_mqtt_user_name = mqtt_user_name;

  return AZ_OK;
}

// <registration_id>
AZ_NODISCARD az_result az_iot_provisioning_client_id_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_client_id,
    az_span* out_mqtt_client_id)
{
  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_client_id, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_client_id);

  int required_length = az_span_length(client->_internal.registration_id);

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(mqtt_client_id, required_length);

  mqtt_client_id = az_span_copy(mqtt_client_id, client->_internal.registration_id);

  *out_mqtt_client_id = mqtt_client_id;

  return AZ_OK;
}

// $dps/registrations/res/#
AZ_NODISCARD az_result az_iot_provisioning_client_register_subscribe_topic_filter_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_topic_filter,
    az_span* out_mqtt_topic_filter)
{
  (void)client;

  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic_filter, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic_filter);

  int32_t required_length = az_span_length(str_dps_registrations_res) + 1;

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(mqtt_topic_filter, required_length);

  mqtt_topic_filter = az_span_copy(mqtt_topic_filter, str_dps_registrations_res);
  mqtt_topic_filter = az_span_append_uint8(mqtt_topic_filter, '#');

  *out_mqtt_topic_filter = mqtt_topic_filter;

  return AZ_OK;
}

AZ_INLINE az_iot_provisioning_client_registration_state registration_state_default()
{
  return (az_iot_provisioning_client_registration_state){ .assigned_hub_hostname = AZ_SPAN_NULL,
                                                          .device_id = AZ_SPAN_NULL,
                                                          .status = AZ_IOT_STATUS_OK,
                                                          .extended_error_code = 0,
                                                          .error_message = AZ_SPAN_NULL,
                                                          .error_tracking_id = AZ_SPAN_NULL,
                                                          .error_timestamp = AZ_SPAN_NULL };
}

/*
Documented at
https://docs.microsoft.com/en-us/rest/api/iot-dps/runtimeregistration/registerdevice#deviceregistrationresult
  "registrationState":{
    "x509":{},
    "registrationId":"paho-sample-device1",
    "createdDateTimeUtc":"2020-04-10T03:11:13.0276997Z",
    "assignedHub":"contoso.azure-devices.net",
    "deviceId":"paho-sample-device1",
    "status":"assigned",
    "substatus":"initialAssignment",
    "lastUpdatedDateTimeUtc":"2020-04-10T03:11:13.2096201Z",
    "etag":"IjYxMDA4ZDQ2LTAwMDAtMDEwMC0wMDAwLTVlOGZlM2QxMDAwMCI="}}
*/
AZ_INLINE az_result az_iot_provisioning_client_payload_registration_state_parse(
    az_json_parser* jp,
    az_json_token_member* tm,
    az_iot_provisioning_client_registration_state* out_state)
{
  if (tm->token.kind != AZ_JSON_TOKEN_OBJECT_START)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  bool assignedHub = false;
  bool deviceId = false;
  bool payload = false;

  while ((!(deviceId && assignedHub && payload))
         && az_succeeded(az_json_parser_parse_token_member(jp, tm)))
  {
    if (az_span_is_content_equal(AZ_SPAN_FROM_STR("assignedHub"), tm->name))
    {
      assignedHub = true;
      AZ_RETURN_IF_FAILED(az_json_token_get_string(&tm->token, &out_state->assigned_hub_hostname));
    }
    else if (az_span_is_content_equal(AZ_SPAN_FROM_STR("deviceId"), tm->name))
    {
      deviceId = true;
      AZ_RETURN_IF_FAILED(az_json_token_get_string(&tm->token, &out_state->device_id));
    }
    else if (az_span_is_content_equal(AZ_SPAN_FROM_STR("payload"), tm->name))
    {
      payload = true;
      uint8_t* start = az_span_ptr(tm->name) + az_span_length(tm->name) + 1;

      if (tm->token.kind == AZ_JSON_TOKEN_OBJECT_START)
      {
        AZ_RETURN_IF_FAILED(az_json_parser_skip_children(jp, tm->token));
      }

      // TODO: temporary
      AZ_RETURN_IF_FAILED(az_json_parser_parse_token_member(jp, tm));


      out_state->json_payload
          = az_span_init(start, (int32_t)(az_span_ptr(tm->name) - start), (int32_t)(az_span_ptr(tm->name) - start));
    }
    else if (tm->token.kind == AZ_JSON_TOKEN_OBJECT_START)
    {
      AZ_RETURN_IF_FAILED(az_json_parser_skip_children(jp, tm->token));
    }
    // else ignore token.
  }

  return AZ_OK;
}

AZ_INLINE az_result az_iot_provisioning_client_payload_parse(
    az_span received_payload,
    az_iot_provisioning_client_register_response* out_response)
{
  // Parse the payload:
  az_json_parser jp;
  az_json_token_member tm;
  bool found_error_code = false;
  bool found_operation_id = false;
  bool found_registration_state = false;

  AZ_RETURN_IF_FAILED(az_json_parser_init(&jp, received_payload));
  AZ_RETURN_IF_FAILED(az_json_parser_parse_token(&jp, &tm.token));
  if (tm.token.kind != AZ_JSON_TOKEN_OBJECT_START)
  {
    return AZ_ERROR_PARSER_UNEXPECTED_CHAR;
  }

  out_response->registration_information = registration_state_default();

  while (az_succeeded(az_json_parser_parse_token_member(&jp, &tm)))
  {
    if (az_span_is_content_equal(AZ_SPAN_FROM_STR("operationId"), tm.name))
    {
      found_operation_id = true;
      AZ_RETURN_IF_FAILED(az_json_token_get_string(&tm.token, &out_response->operation_id));
    }
    else if (az_span_is_content_equal(AZ_SPAN_FROM_STR("status"), tm.name))
    {
      found_registration_state = true;
      AZ_RETURN_IF_FAILED(az_json_token_get_string(&tm.token, &out_response->registration_state));
    }
    else if (az_span_is_content_equal(AZ_SPAN_FROM_STR("registrationState"), tm.name))
    {
      AZ_RETURN_IF_FAILED(az_iot_provisioning_client_payload_registration_state_parse(
          &jp, &tm, &out_response->registration_information));
    }
    else if (az_span_is_content_equal(AZ_SPAN_FROM_STR("errorCode"), tm.name))
    {
      double value;
      AZ_RETURN_IF_FAILED(az_json_token_get_number(&tm.token, &value));
      out_response->registration_information.extended_error_code = (uint32_t)value;
      out_response->registration_information.status = (uint32_t)value / 1000;

      out_response->operation_id = AZ_SPAN_NULL;
      out_response->registration_state = AZ_SPAN_FROM_STR("failed");
      found_error_code = true;
    }
    else if (az_span_is_content_equal(AZ_SPAN_FROM_STR("trackingId"), tm.name))
    {
      AZ_RETURN_IF_FAILED(az_json_token_get_string(
          &tm.token, &out_response->registration_information.error_tracking_id));
    }
    else if (az_span_is_content_equal(AZ_SPAN_FROM_STR("message"), tm.name))
    {
      AZ_RETURN_IF_FAILED(az_json_token_get_string(
          &tm.token, &out_response->registration_information.error_message));
    }
    else if (az_span_is_content_equal(AZ_SPAN_FROM_STR("timestampUtc"), tm.name))
    {
      AZ_RETURN_IF_FAILED(az_json_token_get_string(
          &tm.token, &out_response->registration_information.error_timestamp));
    }
    // else ignore token.
  }

  if (!((found_registration_state && found_operation_id) || found_error_code))
  {
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  return AZ_OK;
}

/*
Example flow:

Stage 1:
 topic: $dps/registrations/res/202/?$rid=1&retry-after=3
 payload:
  {"operationId":"4.d0a671905ea5b2c8.e7173b7b-0e54-4aa0-9d20-aeb1b89e6c7d","status":"assigning"}

Stage 2:
  {"operationId":"4.d0a671905ea5b2c8.e7173b7b-0e54-4aa0-9d20-aeb1b89e6c7d","status":"assigning",
  "registrationState":{"registrationId":"paho-sample-device1","status":"assigning"}}

Stage 3:
 topic: $dps/registrations/res/200/?$rid=1
 payload:
  {"operationId":"4.d0a671905ea5b2c8.e7173b7b-0e54-4aa0-9d20-aeb1b89e6c7d","status":"assigned",
  "registrationState":{ ... }}

 Error:
 topic: $dps/registrations/res/401/?$rid=1
 payload:
 {"errorCode":401002,"trackingId":"8ad0463c-6427-4479-9dfa-3e8bb7003e9b","message":"Invalid
  certificate.","timestampUtc":"2020-04-10T05:24:22.4718526Z"}
*/
AZ_NODISCARD az_result az_iot_provisioning_client_received_topic_payload_parse(
    az_iot_provisioning_client const* client,
    az_span received_topic,
    az_span received_payload,
    az_iot_provisioning_client_register_response* out_response)
{
  (void)client;

  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(received_topic, 0, false);
  AZ_PRECONDITION_VALID_SPAN(received_payload, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_response);

  int32_t idx = az_span_find(received_topic, str_dps_registrations_res);
  if (idx != 0)
  {
    // TODO: Replace with common IoT topic not matched.
    return AZ_ERROR_ITEM_NOT_FOUND;
  }

  // Parse the status.
  az_span remainder = az_span_slice(
      received_topic, az_span_length(str_dps_registrations_res), az_span_length(received_topic));

  az_span int_slice = az_span_token(remainder, AZ_SPAN_FROM_STR("/"), &remainder);
  AZ_RETURN_IF_FAILED(az_span_to_uint32(int_slice, (uint32_t*)(&out_response->status)));

  // Parse the optional retry-after= field.
  az_span retry_after = AZ_SPAN_FROM_STR("retry-after=");
  idx = az_span_find(remainder, retry_after);
  if (idx != -1)
  {
    remainder
        = az_span_slice(remainder, idx + az_span_length(retry_after), az_span_length(remainder));
    int_slice = az_span_token(remainder, AZ_SPAN_FROM_STR("&"), &remainder);

    AZ_RETURN_IF_FAILED(az_span_to_uint32(int_slice, &out_response->retry_after_seconds));
  }
  else
  {
    out_response->retry_after_seconds = 0;
  }

  AZ_RETURN_IF_FAILED(az_iot_provisioning_client_payload_parse(received_payload, out_response));

  return AZ_OK;
}

// $dps/registrations/PUT/iotdps-register/?$rid=%s
AZ_NODISCARD az_result az_iot_provisioning_client_register_publish_topic_get(
    az_iot_provisioning_client const* client,
    az_span mqtt_topic,
    az_span* out_mqtt_topic)
{
  (void)client;

  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);

  az_span str_dps_registrations = az_span_slice(str_dps_registrations_res, 0, idx_res_start);

  int32_t required_length = az_span_length(str_dps_registrations_res)
      + az_span_length(str_dps_registrations) + az_span_length(str_put_iotdps_register);

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(mqtt_topic, required_length);
  mqtt_topic = az_span_copy(mqtt_topic, str_dps_registrations);
  mqtt_topic = az_span_append(mqtt_topic, str_put_iotdps_register);

  *out_mqtt_topic = mqtt_topic;

  return AZ_OK;
}

// Topic: $dps/registrations/GET/iotdps-get-operationstatus/?$rid=%s&operationId=%s
AZ_NODISCARD az_result az_iot_provisioning_client_get_operation_status_publish_topic_get(
    az_iot_provisioning_client const* client,
    az_iot_provisioning_client_register_response const* register_response,
    az_span mqtt_topic,
    az_span* out_mqtt_topic)
{
  (void)client;

  AZ_PRECONDITION_NOT_NULL(client);
  AZ_PRECONDITION_VALID_SPAN(mqtt_topic, 0, false);
  AZ_PRECONDITION_NOT_NULL(out_mqtt_topic);
  AZ_PRECONDITION_NOT_NULL(register_response);
  AZ_PRECONDITION_VALID_SPAN(register_response->operation_id, 1, false);

  az_span str_dps_registrations = az_span_slice(str_dps_registrations_res, 0, idx_res_start);

  int32_t required_length = az_span_length(str_dps_registrations)
      + az_span_length(str_get_iotdps_get_operationstatus)
      + az_span_length(register_response->operation_id);

  AZ_RETURN_IF_NOT_ENOUGH_CAPACITY(mqtt_topic, required_length);

  mqtt_topic = az_span_copy(mqtt_topic, str_dps_registrations);
  mqtt_topic = az_span_append(mqtt_topic, str_get_iotdps_get_operationstatus);
  mqtt_topic = az_span_append(mqtt_topic, register_response->operation_id);

  *out_mqtt_topic = mqtt_topic;

  return AZ_OK;
}
