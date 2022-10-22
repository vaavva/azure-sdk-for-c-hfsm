// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license
// information.

/**
 * @file csdk_iothub_device_client_ll_adapter.c
 * @brief C-SDK Compat implementation for the IoT Hub LL client.
 * @details The following APIs rely on heap allocations (malloc/free) to create objects.
 *
 */

// HFSM_TODO: This implementation is proof-of-concept only.

#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mosquitto.h"

#include <azure/az_iot.h>
#include <azure/core/az_hfsm_pipeline.h>
#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/iot/internal/az_iot_hub_hfsm.h>

#include <azure/iot/compat/iothub.h>
#include <azure/iot/compat/iothub_client_options.h>
#include <azure/iot/compat/iothub_device_client_ll.h>
#include <azure/iot/compat/iothub_transport_ll.h>

#include <azure/iot/compat/internal/az_compat_csdk.h>

#define SYS_PROP_MESSAGE_ID "mid"
#define SYS_PROP_MESSAGE_CREATION_TIME_UTC "ctime"
#define SYS_PROP_USER_ID "uid"
#define SYS_PROP_CORRELATION_ID "cid"
#define SYS_PROP_CONTENT_TYPE "ct"
#define SYS_PROP_CONTENT_ENCODING "ce"
#define SYS_PROP_DIAGNOSTIC_ID "diagid"
#define SYS_PROP_DIAGNOSTIC_CONTEXT "diagctx"
#define SYS_PROP_CONNECTION_DEVICE_ID "cdid"
#define SYS_PROP_CONNECTION_MODULE_ID "cmid"
#define SYS_PROP_ON "on"
#define SYS_PROP_EXP "exp"
#define SYS_PROP_TO "to"
#define SYS_COMPONENT_NAME "sub"

typedef struct az_hfsm_compat_csdk_telemetry_req_data az_hfsm_compat_csdk_telemetry_req_data;

#define Q_TYPE az_hfsm_compat_csdk_telemetry_req_data*
#define Q_SIZE AZ_IOT_COMPAT_CSDK_MAX_QUEUE_SIZE
#include <azure/iot/compat/internal/queue.h>

typedef struct IOTHUB_CLIENT_CORE_LL_HANDLE_DATA_TAG
{
  // HFSM for the Compat Client
  az_hfsm_policy compat_client_policy;

  // Endpoint information
  az_span hub_endpoint;
  az_span device_id;
  az_span cert_path;
  az_span key_path;

  // Hub Pipeline
  az_hfsm_pipeline hub_pipeline;
  az_hfsm_iot_hub_policy hub_policy;
  az_iot_hub_client hub_client;
  az_hfsm_mqtt_policy hub_mqtt_policy;

  char hub_endpoint_buffer[AZ_IOT_MAX_HUB_NAME_SIZE];
  char topic_buffer[AZ_IOT_MAX_TOPIC_SIZE];
  char username_buffer[AZ_IOT_MAX_USERNAME_SIZE];
  char password_buffer[AZ_IOT_MAX_PASSWORD_SIZE];
  char client_id_buffer[AZ_IOT_MAX_CLIENT_ID_SIZE];

  queue pub_message_queue;
  queue puback_message_queue;

  IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connection_status_callback;
  void* connection_status_callback_user_context;

  IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC c2d_message_callback;
  void* c2d_message_callback_user_context;

} IOTHUB_CLIENT_CORE_LL_HANDLE_DATA;

typedef struct IOTHUB_MESSAGE_HANDLE_DATA_TAG
{
  int32_t refcount;
  az_hfsm_iot_hub_telemetry_data telemetry_data;
} IOTHUB_MESSAGE_HANDLE_DATA;

const TRANSPORT_PROVIDER* MQTT_Protocol(void) { return (TRANSPORT_PROVIDER*)0x4D515454; }

// ***** Single-layer C-SDK Compat Layer State Machine
struct az_hfsm_compat_csdk_telemetry_req_data
{
  IOTHUB_MESSAGE_HANDLE_DATA* message;
  IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK callback;
  void* userContextCallback;
  // The packet ID is copied because message could be re-used by the application.
  int32_t out_packet_id;
};

static az_result root(az_hfsm* me, az_hfsm_event event);
static az_result disconnected(az_hfsm* me, az_hfsm_event event);
static az_result connecting(az_hfsm* me, az_hfsm_event event);
static az_result connected(az_hfsm* me, az_hfsm_event event);

static az_hfsm_state_handler _get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else if (child_state == disconnected || child_state == connecting || child_state == connected)
  {
    parent_state = root;
  }
  else
  {
    // Unknown state.
    az_platform_critical_error();
    parent_state = NULL;
  }

  return parent_state;
}

static az_result root(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("compat-csdk-hub/root"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op.
      break;

    case AZ_HFSM_EVENT_EXIT:
    case AZ_HFSM_EVENT_ERROR:
    {
      az_hfsm_event_data_error* err_data = (az_hfsm_event_data_error*)event.data;
      printf(
          LOG_COMPAT "\x1B[31mERROR\x1B[0m: IoT Hub Client %p az_result=%s (%x) hfsm=%p\n",
          me,
          az_result_string(err_data->error_type),
          err_data->error_type,
          err_data->origin);
      break;
    }

    // Pass-through events.
    case AZ_IOT_HUB_DISCONNECT_REQ:
    case AZ_HFSM_EVENT_TIMEOUT:
      // Pass-through events.
      ret = az_hfsm_pipeline_send_outbound_event((az_hfsm_pipeline*)client, event);
      break;

    default:
      printf(LOG_COMPAT "UNKNOWN event! %x\n", event.type);
      az_platform_critical_error();
      break;
  }

  return ret;
}

AZ_INLINE az_result _connect(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client)
{
  az_hfsm_iot_x509_auth auth = (az_hfsm_iot_x509_auth){
    .cert = client->cert_path,
    .key = client->key_path,
  };
  az_hfsm_iot_hub_connect_data connect_data = (az_hfsm_iot_hub_connect_data){
    .auth = auth,
    .auth_type = AZ_HFSM_IOT_AUTH_X509,
    .client_id_buffer = AZ_SPAN_FROM_BUFFER(client->client_id_buffer),
    .username_buffer = AZ_SPAN_FROM_BUFFER(client->username_buffer),
    .password_buffer = AZ_SPAN_FROM_BUFFER(client->password_buffer),
  };

  return az_hfsm_send_event(
      (az_hfsm*)&client->hub_policy, (az_hfsm_event){ AZ_IOT_HUB_CONNECT_REQ, &connect_data });
}

static az_result disconnected(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("compat-csdk-hub/root/disconnected"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP:
      _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, disconnected, connecting));
      _az_RETURN_IF_FAILED(_connect(client));
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE void _connection_status_callback(
    IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* me,
    IOTHUB_CLIENT_CONNECTION_STATUS result,
    IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason)
{
  if (me->connection_status_callback != NULL)
  {
    me->connection_status_callback(result, reason, me->connection_status_callback_user_context);
  }
}

AZ_INLINE az_result _handle_disconnect(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client)
{
  _connection_status_callback(
      client, IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED, IOTHUB_CLIENT_CONNECTION_NO_NETWORK);
  return AZ_OK;
}

static az_result connecting(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("compat-csdk-hub/root/connecting"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
    case AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP:
      // No-op.
      break;

    case AZ_IOT_HUB_CONNECT_RSP:
      ret = az_hfsm_transition_peer(me, connecting, connected);
      _connection_status_callback(
          client, IOTHUB_CLIENT_CONNECTION_AUTHENTICATED, IOTHUB_CLIENT_CONNECTION_OK);
      break;

    case AZ_IOT_HUB_DISCONNECT_RSP:
    {
      _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, connecting, disconnected));
      _az_RETURN_IF_FAILED(_handle_disconnect(client));
      break;
    }

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

AZ_INLINE az_result _process_outbound(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* me)
{
  while (me->pub_message_queue.count > 0
         && AZ_IOT_COMPAT_CSDK_MAX_QUEUE_SIZE - me->puback_message_queue.count > 0)
  {
    az_hfsm_compat_csdk_telemetry_req_data* message;
    _az_RETURN_IF_FAILED(queue_peek(&me->pub_message_queue, &message));

    _az_RETURN_IF_FAILED(az_hfsm_pipeline_send_outbound_event(
        (az_hfsm_policy*)me,
        (az_hfsm_event){ AZ_IOT_HUB_TELEMETRY_REQ, &message->message->telemetry_data }));

    _az_RETURN_IF_FAILED(queue_dequeue(&me->pub_message_queue, &message));

    // If the I/O succeeded, copy the message ID in case the Application decides to reuse the
    // message object.
    message->out_packet_id = message->message->telemetry_data.out_packet_id;

    _az_RETURN_IF_FAILED(queue_enqueue(&me->puback_message_queue, message));
  }

  return AZ_OK;
}

AZ_INLINE void _telemetry_c2d_message_callback(
    IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* me,
    az_hfsm_compat_csdk_telemetry_req_data* request)
{
  if (request->callback != NULL)
  {
    // Call the per-message application send message callback.
    request->callback(IOTHUB_CLIENT_CONFIRMATION_OK, request->userContextCallback);
  }
}

AZ_INLINE void _c2d_c2d_message_callback(
    IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* me,
    az_hfsm_iot_hub_c2d_request_data* request)
{
  if (me->c2d_message_callback != NULL)
  {
    IOTHUB_MESSAGE_HANDLE_DATA message = (IOTHUB_MESSAGE_HANDLE_DATA){
      .refcount = 1,
      .telemetry_data.data = request->payload,
      .telemetry_data.topic_buffer = AZ_SPAN_EMPTY,
      .telemetry_data.out_packet_id = 0,
      .telemetry_data.properties = &request->topic_info.properties,
    };

    IOTHUBMESSAGE_DISPOSITION_RESULT ret
        = me->c2d_message_callback(&message, me->c2d_message_callback_user_context);
  }
}

AZ_INLINE az_result
_process_puback(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* me, az_hfsm_mqtt_puback_data* data)
{
  az_hfsm_compat_csdk_telemetry_req_data* request;
  _az_RETURN_IF_FAILED(queue_peek(&me->puback_message_queue, &request));

  if (request->out_packet_id == data->id)
  {
    _az_RETURN_IF_FAILED(queue_dequeue(&me->puback_message_queue, &request));
    _telemetry_c2d_message_callback(me, request);

    IoTHubMessage_Destroy(request->message);
    free(request);
  }
  else if (request->out_packet_id < data->id)
  {
    // MQTT protocol violation: PUBACK must be acknowledge in order.
    az_platform_critical_error();
  }

  return AZ_OK;
}

static az_result connected(az_hfsm* me, az_hfsm_event event)
{
  az_result ret = AZ_OK;
  IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client = (IOTHUB_CLIENT_CORE_LL_HANDLE_DATA*)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("compat-csdk-hub/root/connected"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
    case AZ_HFSM_EVENT_EXIT:
      // No-op.
      break;

    case AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP:
      ret = _process_outbound(client);
      break;

    case AZ_IOT_HUB_DISCONNECT_RSP:
      _az_RETURN_IF_FAILED(az_hfsm_transition_peer(me, connected, disconnected));
      _az_RETURN_IF_FAILED(_handle_disconnect(client));
      break;

    case AZ_HFSM_MQTT_EVENT_PUBACK_RSP:
    {
      az_hfsm_mqtt_puback_data* puback = (az_hfsm_mqtt_puback_data*)event.data;
      ret = _process_puback(client, puback);
      break;
    }

    case AZ_IOT_HUB_C2D_REQ:
    {
      az_hfsm_iot_hub_c2d_request_data* c2d_req = (az_hfsm_iot_hub_c2d_request_data*)event.data;
      _c2d_c2d_message_callback(client, c2d_req);
      break;
    }

    case AZ_IOT_HUB_METHODS_REQ:
    {
      az_hfsm_iot_hub_method_request_data* method_req
          = (az_hfsm_iot_hub_method_request_data*)event.data;
      printf(
          LOG_COMPAT "HUB: Method received [%.*s]\n",
          az_span_size(method_req->name),
          az_span_ptr(method_req->name));

      // Callback (should be set).

      az_hfsm_iot_hub_method_response_data method_rsp
          = (az_hfsm_iot_hub_method_response_data){ .status = 0,
                                                    .request_id = method_req->request_id,
                                                    .topic_buffer
                                                    = AZ_SPAN_FROM_BUFFER(client->topic_buffer),
                                                    .payload = AZ_SPAN_EMPTY };

      ret = az_hfsm_send_event(
          (az_hfsm*)&client->hub_policy, (az_hfsm_event){ AZ_IOT_HUB_METHODS_RSP, &method_rsp });
    }
    break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result hub_initialize(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client)
{
  client->compat_client_policy = (az_hfsm_policy){
    .inbound = NULL,
    .outbound = (az_hfsm_policy*)&client->hub_policy,
  };

  _az_RETURN_IF_FAILED(az_hfsm_init((az_hfsm*)&client->compat_client_policy, root, _get_parent));
  _az_RETURN_IF_FAILED(
      az_hfsm_transition_substate((az_hfsm*)&client->compat_client_policy, root, disconnected));

  _az_RETURN_IF_FAILED(az_hfsm_pipeline_init(
      &client->hub_pipeline,
      (az_hfsm_policy*)&client->compat_client_policy,
      (az_hfsm_policy*)&client->hub_mqtt_policy));

  _az_RETURN_IF_FAILED(
      az_iot_hub_client_init(&client->hub_client, client->hub_endpoint, client->device_id, NULL));

  _az_RETURN_IF_FAILED(az_hfsm_iot_hub_policy_initialize(
      &client->hub_policy,
      &client->hub_pipeline,
      &client->compat_client_policy,
      (az_hfsm_policy*)&client->hub_mqtt_policy,
      &client->hub_client,
      NULL));

  // MQTT
  // HFSM_DESIGN: The Trusted Certificates are set within SetOption
  _az_RETURN_IF_FAILED(az_mqtt_initialize(
      &client->hub_mqtt_policy, &client->hub_pipeline, (az_hfsm_policy*)&client->hub_policy, NULL));

  return AZ_OK;
}

// ************************************* C-SDK Compat layer ************************************ //

const char* IoTHubClient_GetVersionString() { return "2.0.0-alpha"; }

int IoTHub_Init() { _az_RETURN_IF_FAILED(az_mqtt_init()); }

void IoTHub_Deinit()
{
  if (az_result_failed(az_mqtt_deinit()))
  {
    az_platform_critical_error();
  }
}

static AZ_NODISCARD az_result _csdk_iothub_client_init(
    IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client,
    az_span hub_endpoint,
    az_span device_id)
{
  client->hub_endpoint = hub_endpoint;
  client->device_id = device_id;

  queue_init(&client->pub_message_queue);
  queue_init(&client->puback_message_queue);

  return hub_initialize(client);
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE IoTHubDeviceClient_LL_Create(const IOTHUB_CLIENT_CONFIG* config)
{
  _az_PRECONDITION(config->protocol == (IOTHUB_CLIENT_TRANSPORT_PROVIDER)MQTT_Protocol);

  az_result ret;
  IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client = malloc(sizeof(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA));

  if (client != NULL)
  {
    memset(client, 0, sizeof(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA));

    // Construct contiguous hub_endpoint name from IOTHUB_CLIENT_CONFIG.
    size_t hub_endpoint_size = strlen(config->iotHubName) + strlen(config->iotHubSuffix) + 1;
    client->hub_endpoint = AZ_SPAN_FROM_BUFFER(client->hub_endpoint_buffer);
    az_span reminder = az_span_copy(
        client->hub_endpoint, az_span_create_from_str((char*)(uintptr_t)config->iotHubName));
    reminder = az_span_copy_u8(reminder, '.');
    reminder
        = az_span_copy(reminder, az_span_create_from_str((char*)(uintptr_t)config->iotHubSuffix));

    // HFSM_TODO: client_options (e.g. moduleID)

    if (az_result_failed(_csdk_iothub_client_init(
            client,
            az_span_slice(client->hub_endpoint, 0, hub_endpoint_size),
            az_span_create_from_str((char*)(uintptr_t)config->deviceId))))
    {
      free(client);
      client = NULL;
    }
  }

  return client;
}

IOTHUB_DEVICE_CLIENT_LL_HANDLE IoTHubDeviceClient_LL_CreateFromDeviceAuth(
    const char* iothub_uri,
    const char* device_id,
    IOTHUB_CLIENT_TRANSPORT_PROVIDER protocol)
{
  _az_PRECONDITION(protocol == (IOTHUB_CLIENT_TRANSPORT_PROVIDER)MQTT_Protocol);

  az_result ret;
  IOTHUB_CLIENT_CORE_LL_HANDLE_DATA* client = malloc(sizeof(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA));

  if (client != NULL)
  {
    memset(client, 0, sizeof(IOTHUB_CLIENT_CORE_LL_HANDLE_DATA));
    // HFSM_TODO: client_options (e.g. moduleID)

    if (az_result_failed(_csdk_iothub_client_init(
            client,
            az_span_create_from_str((char*)(uintptr_t)iothub_uri),
            az_span_create_from_str((char*)(uintptr_t)device_id))))
    {
      free(client);
      client = NULL;
    }

    // Configure the client based on the HSM interface.
    client->cert_path = security_provider.cert;
    client->key_path = security_provider.key;
  }

  return client;
}

void IoTHubDeviceClient_LL_Destroy(IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle)
{
  // Disconnect async (disconnect will be initiated here).
  if (az_result_failed(az_hfsm_pipeline_post_outbound_event(
          &iotHubClientHandle->hub_pipeline, (az_hfsm_event){ AZ_IOT_HUB_DISCONNECT_REQ, NULL })))
  {
    az_platform_critical_error();
  }

  free(iotHubClientHandle);
}

IOTHUB_CLIENT_RESULT IoTHubDeviceClient_LL_SetOption(
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle,
    const char* optionName,
    const void* value)
{
  int result;

  if ((iotHubClientHandle == NULL) || (optionName == NULL) || (value == NULL))
  {
    result = IOTHUB_CLIENT_INVALID_ARG;
  }
  else if (!strcmp(optionName, OPTION_LOG_TRACE))
  {
    if (*(bool*)value == true)
    {
      az_log_set_message_callback(az_sdk_log_callback);
      az_log_set_classification_filter_callback(az_sdk_log_filter_callback);
    }
  }
  else if (!strcmp(optionName, OPTION_AUTO_URL_ENCODE_DECODE))
  {
    // HFSM_TODO: not implemented.
    result = IOTHUB_CLIENT_OK;
  }
  else if (!strcmp(optionName, OPTION_OPENSSL_ENGINE))
  {
    // HFSM_TODO: not implemented.
    result = IOTHUB_CLIENT_ERROR;
  }
  else if (!strcmp(optionName, OPTION_OPENSSL_PRIVATE_KEY_TYPE))
  {
    // HFSM_TODO: not implemented.
    result = IOTHUB_CLIENT_ERROR;
  }
  else if (!strcmp(optionName, OPTION_X509_CERT))
  {
    iotHubClientHandle->cert_path = az_span_create_from_str((char*)(uintptr_t)value);
    result = IOTHUB_CLIENT_OK;
  }
  else if (!strcmp(optionName, OPTION_X509_PRIVATE_KEY))
  {
    iotHubClientHandle->key_path = az_span_create_from_str((char*)(uintptr_t)value);
    result = IOTHUB_CLIENT_OK;
  }
  else if (!strcmp(optionName, OPTION_TRUSTED_CERT))
  {
    // HFSM_TODO: keep CA certs for MQTT policy only.
    iotHubClientHandle->hub_mqtt_policy._internal.options.certificate_authority_trusted_roots
        = az_span_create_from_str((char*)(uintptr_t)value);
    result = IOTHUB_CLIENT_OK;
  }
  else
  {
    result = IOTHUB_CLIENT_ERROR;
  }

  return result;
}

IOTHUB_CLIENT_RESULT IoTHubDeviceClient_LL_SendEventAsync(
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle,
    IOTHUB_MESSAGE_HANDLE eventMessageHandle,
    IOTHUB_CLIENT_EVENT_CONFIRMATION_CALLBACK eventConfirmationCallback,
    void* userContextCallback)
{
  IOTHUB_CLIENT_RESULT ret;

  az_hfsm_compat_csdk_telemetry_req_data* request
      = malloc(sizeof(az_hfsm_compat_csdk_telemetry_req_data));
  if (request == NULL)
  {
    return IOTHUB_CLIENT_ERROR;
  }

  request->message = eventMessageHandle;
  request->message->refcount++;
  request->callback = eventConfirmationCallback;
  request->userContextCallback = userContextCallback;
  request->out_packet_id = -1;

  if (az_result_failed(queue_enqueue(&iotHubClientHandle->pub_message_queue, request)))
  {
    return IOTHUB_CLIENT_ERROR;
  }

  return IOTHUB_CLIENT_OK;
}

void IoTHubDeviceClient_LL_DoWork(IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle)
{
  az_result ret = az_hfsm_pipeline_syncrhonous_process_loop(&iotHubClientHandle->hub_pipeline);
  if (az_result_failed(ret))
  {
    _connection_status_callback(
        iotHubClientHandle,
        IOTHUB_CLIENT_CONNECTION_UNAUTHENTICATED,
        IOTHUB_CLIENT_CONNECTION_COMMUNICATION_ERROR);

    printf(LOG_COMPAT "Process Loop Failed: %s (%x)\n", az_result_string(ret), ret);
  }
}

IOTHUB_CLIENT_RESULT IoTHubDeviceClient_LL_SetConnectionStatusCallback(
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle,
    IOTHUB_CLIENT_CONNECTION_STATUS_CALLBACK connectionStatusCallback,
    void* userContextCallback)
{
  // HFSM_TODO: error checking.
  iotHubClientHandle->connection_status_callback = connectionStatusCallback;
  iotHubClientHandle->connection_status_callback_user_context = userContextCallback;

  return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT IoTHubDeviceClient_LL_SetMessageCallback(
    IOTHUB_DEVICE_CLIENT_LL_HANDLE iotHubClientHandle,
    IOTHUB_CLIENT_MESSAGE_CALLBACK_ASYNC messageCallback,
    void* userContextCallback)
{
  iotHubClientHandle->c2d_message_callback = messageCallback;
  iotHubClientHandle->c2d_message_callback_user_context = userContextCallback;

  return IOTHUB_CLIENT_OK;
}

// ************************************* C-SDK Message  **************************************** //

IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromByteArray(const unsigned char* byteArray, size_t size)
{
  IOTHUB_MESSAGE_HANDLE_DATA* msg = malloc(sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
  if (msg != NULL)
  {
    memset(msg, 0, sizeof(IOTHUB_MESSAGE_HANDLE_DATA));
  }

  az_iot_message_properties* properties = malloc(sizeof(az_iot_message_properties));
  char* topic_buffer = malloc(AZ_IOT_MAX_TOPIC_SIZE);
  char* properties_buffer = malloc(AZ_IOT_MAX_TOPIC_SIZE);
  char* data_buffer = malloc(size);

  if (msg != NULL && properties != NULL && topic_buffer != NULL && properties_buffer != NULL
      && data_buffer != NULL
      && az_result_succeeded(az_iot_message_properties_init(
          properties, az_span_create(properties_buffer, AZ_IOT_MAX_TOPIC_SIZE), 0)))
  {
    msg->refcount = 1;
    msg->telemetry_data.properties = properties;
    msg->telemetry_data.topic_buffer = az_span_create(topic_buffer, AZ_IOT_MAX_TOPIC_SIZE);
    memcpy(data_buffer, byteArray, size);

    msg->telemetry_data.data = az_span_create(data_buffer, size);
    msg->telemetry_data.out_packet_id = -1;
  }
  else
  {
    IoTHubMessage_Destroy(msg);
    msg = NULL;
  }

  return msg;
}

IOTHUB_MESSAGE_HANDLE IoTHubMessage_CreateFromString(const char* source)
{
  size_t data_size = strlen(source) + 1;
  return IoTHubMessage_CreateFromByteArray(source, data_size);
}

void IoTHubMessage_Destroy(IOTHUB_MESSAGE_HANDLE iotHubMessageHandle)
{
  IOTHUB_MESSAGE_HANDLE_DATA* msg = iotHubMessageHandle;
  if (msg != NULL)
  {
    msg->refcount--;

    if (msg->refcount <= 0)
    {
      if (az_span_ptr(msg->telemetry_data.topic_buffer) != NULL)
      {
        free(az_span_ptr(msg->telemetry_data.topic_buffer));
        msg->telemetry_data.topic_buffer = AZ_SPAN_EMPTY;
      }

      if (az_span_ptr(msg->telemetry_data.data) != NULL)
      {
        free(az_span_ptr(msg->telemetry_data.data));
        msg->telemetry_data.topic_buffer = AZ_SPAN_EMPTY;
      }

      if (az_span_ptr(msg->telemetry_data.properties->_internal.properties_buffer))
      {
        free(az_span_ptr(msg->telemetry_data.properties->_internal.properties_buffer));
      }

      free(msg);
    }
  }
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetMessageId(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* messageId)
{
  return IoTHubMessage_SetProperty(iotHubMessageHandle, SYS_PROP_MESSAGE_ID, messageId);
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetCorrelationId(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* correlationId)
{
  return IoTHubMessage_SetProperty(iotHubMessageHandle, SYS_PROP_CORRELATION_ID, correlationId);
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentTypeSystemProperty(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* contentType)
{
  return IoTHubMessage_SetProperty(iotHubMessageHandle, SYS_PROP_CONTENT_TYPE, contentType);
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetContentEncodingSystemProperty(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* contentEncoding)
{
  return IoTHubMessage_SetProperty(iotHubMessageHandle, SYS_PROP_CONTENT_ENCODING, contentEncoding);
}

IOTHUB_MESSAGE_RESULT IoTHubMessage_SetProperty(
    IOTHUB_MESSAGE_HANDLE iotHubMessageHandle,
    const char* key,
    const char* value)
{
  IOTHUB_MESSAGE_HANDLE_DATA* msg = iotHubMessageHandle;

  if (az_result_failed(az_iot_message_properties_append(
          msg->telemetry_data.properties,
          az_span_create_from_str((char*)(uintptr_t)key),
          az_span_create_from_str((char*)(uintptr_t)value))))
  {
    return IOTHUB_MESSAGE_ERROR;
  }

  return IOTHUB_MESSAGE_OK;
}
