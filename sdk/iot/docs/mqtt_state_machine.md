# Azure IoT SDK MQTT State Machine

## High-level architecture

Device Provisioning and IoT Hub service protocols require additional state management on top of the MQTT protocol. The Azure IoT SDK for C provides a common programming model layered on an MQTT client selected by the application developer.

The following aspects are being handled by the SDK:
1. Generate MQTT CONNECT credentials.
1. Obtain SUBSCRIBE topic filters and PUBLISH topic strings required by various service features.
1. Parse service errors and output an uniform error object model (i.e. az_result) 
1. Provide the correct sequence of events required to perform an operation. 
1. Provide suggested timing information when retrying operations.

The following aspects need to be handled by the application or convenience layers:
1. Ensure secure TLS communication using either server or mutual X509 authentication.
1. Perform MQTT transport-level operations.
1. Delay execution for retry purposes.
1. (Optional) Provide real-time clock information and perform HMAC-SHA256 operations for SAS token generation.

## Components

### IoT Hub
![image](iot_hub_flow.png "IoT MQTT State Machine")

### Device Provisioning Service
![image](iot_provisioning_flow.png "Device Provisioning MQTT State Machine")

## Design Decisions
Porting requirements:
- The target platform C99 compiler is generating reentrant code. The target platform supports a stack of several kB.
- Our SDK relies on types such as `uint8_t` existing. If a platform doesn't already have them defined, they should be added to the application code using `typedef` statements.

The SDK is provided only for the MQTT protocol. Support for WebSocket/WebProxy tunneling as well as TLS security is not handled by the SDK.

Assumptions and recommendations for the application:
- Our API does not support unsubscribing from any of the previously subscribed topics. We assume that the device will only set-up topics that must be used.

## API

### Connecting

The application code is required to initialize the TLS and MQTT stacks.
Two authentication schemes are currently supported: _X509 Client Certificate Authentication_ and _Shared Access Signature_ authentication. 

When X509 client authenticaiton is used, the MQTT password field should be an empty string. The following API can be used to obtain the MQTT username:

```C
// Provisioning
az_result az_iot_provisioning_user_name_get(az_span id_scope, az_span registration_id, az_span user_agent, az_span mqtt_user_name, az_span* out_mqtt_user_name);

// IoT Hub

void az_iot_hub_identity_init(az_iot_identity *identity, az_span device_id, az_span module_id);
az_result az_iot_hub_user_name_get(az_span iot_hub, az_iot_identity* identity, az_span user_agent, az_span mqtt_user_name, az_span* out_mqtt_user_name);
```

If SAS tokens are used the following APIs provide a way to create as well as refresh the lifetime of the used token upon reconnect:

```C
// Provisioning
az_result az_iot_provisioning_sas_init(az_iot_provisioning_sas* sas, az_span id_scope, az_span registration_id, az_span optional_key_name, az_span signature_buffer);
az_result az_iot_provisioning_sas_update_signature(az_iot_provisioning_sas* sas, uint32_t token_expiration_unix_time, az_span to_hmac_sha256_sign);
az_result az_iot_provisioning_sas_update_password(az_iot_provisioning_sas* sas, az_span base64_hmac_sha256_signature, az_span mqtt_password, az_span* out_mqtt_password);

// IoT Hub
az_result az_iot_hub_sas_init(az_iot_hub_sas* sas, az_iot_identity* identity, az_span hub_name, az_span key_name, az_span signature_buffer);
az_result az_iot_hub_sas_update_signature(az_iot_hub_sas* sas, uint32_t token_expiration_unix_time, az_span to_hmac_sha256_sign);
az_result az_iot_hub_sas_update_password(az_iot_hub_sas* sas, az_span base64_hmac_sha256_signature, az_span mqtt_password, az_span* out_mqtt_password);
```

### Subscribe topic information
Note: Azure IoT services (Device Provisioning, IoT Hub) require either no subscription (e.g. sending device-to-cloud Telemetry) or _a single_ subscription to a topic _filter_.

Each service requiring subscriptions is componentized and must implement a function similar to the following:

_Examples:_
```C
// Provisioning:
az_result az_iot_provisioning_register_subscribe_topic_filter_get(az_span mqtt_topic_filter, az_span* out_mqtt_topic_filter);

// IoT Hub:
az_result az_iot_hub_methods_subscribe_topic_filter_get(az_span mqtt_topic_filter, az_span* out_mqtt_topic_filter);
```

### Sending APIs

Each action (e.g. send telemetry, request twin) is represented by a separate public API.
The application is responsible for filling in the MQTT payload with the format expected by the service.

_Examples:_
```C
// Provisioning
// MQTT payload must be a valid JSON string.
az_result az_iot_provisioning_register_publish_topic_get(az_span registration_id, az_span mqtt_topic, az_span *out_mqtt_topic);

// IoT Hub:
// Accepts binary MQTT payloads.
az_result az_iot_telemetry_publish_topic_get(az_iot_identity* identity, az_span properties, az_span mqtt_topic, az_span *out_mqtt_topic);
```

### Receiving APIs

We recommend that the handling of incoming MQTT PUB messages is implemented by a delegating handler architecture. Each handler is passed the topic and will 

Example MQTT PUB handler implementation:

```C
    az_result ret;
    az_iot_c2d_request c2d_request;
    az_iot_method_request method_request;
    az_iot_twin_response twin_response;
    
    // TODO: There are 2 failure modes: az_result_iot_not_handled which is expected and other errors.
    // Proposed solutions:
    // 1. use logging from the SDK
    // 2. create an app function az_iot_handled (similar to az_succeded but that prints out errors)
    if (az_succeeded(ret = az_iot_c2d_handle(&iot_client, &pub_received, &c2d_request)))
    {
        // c2d_request contains .payload and .properties / count
    }
    else if (az_succeeded(ret = az_iot_methods_handle(&iot_client, &pub_received, &method_request)))
    {
        // method received: method_request.name
    }
    else if (az_succeeded(ret = az_iot_twin_handle(&iot_client, &pub_received, &twin_response)))
    {
        // process twin update.
    }
```

## Error Handling and Retry

### Common Retry Flow
![image](iot_retry_flow.png "MQTT Retry Flow")


## Sample Application

* [Paho Device Provisioning Sample](iot_provisioning_paho_sample.c)
* [Paho IoT Hub Sample](iot_hub_paho_sample.c)

## Test plan

Testing the programming model is achieved by using the APIs with multiple MQTT clients to ensure all scenarios can be achieved.
Describing concrete implementations of the APIs is out of scope for this document.

All APIs must be tested for both normal operation as well as error cases. This is achieved in isolation (Unit Testing) by varying inputs and verifying outputs of the above-described APIs. 

E2E Functional testing is required to ensure that the assumptions are correct and that the designed client is compatible with current versions of the Azure IoT services.
