// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*

/home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:111: undefined reference to `MQTT_Protocol'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:129: undefined reference to `IoTHub_Init'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:133: undefined reference to `IoTHubDeviceClient_LL_CreateFromConnectionString'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:156: undefined reference to `IoTHubDeviceClient_LL_SetOption'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:164: undefined reference to `IoTHubDeviceClient_LL_SetOption'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:165: undefined reference to `IoTHubDeviceClient_LL_SetOption'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:177: undefined reference to `IoTHubMessage_CreateFromString'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:181: undefined reference to `IoTHubMessage_SetMessageId'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:182: undefined reference to `IoTHubMessage_SetCorrelationId'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:183: undefined reference to `IoTHubMessage_SetContentTypeSystemProperty'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:184: undefined reference to `IoTHubMessage_SetContentEncodingSystemProperty'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:187: undefined reference to `IoTHubMessage_SetProperty'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:190: undefined reference to `IoTHubDeviceClient_LL_SendEventAsync'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:193: undefined reference to `IoTHubMessage_Destroy'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:203: undefined reference to `IoTHubDeviceClient_LL_DoWork'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:209: undefined reference to `IoTHubDeviceClient_LL_Destroy'
/usr/bin/ld: /home/crispop/c/sdk/samples/iot_compat/iothub_ll_client_x509_sample.c:212: undefined reference to `IoTHub_Deinit'

*/

#include "iothub.h"

const TRANSPORT_PROVIDER* MQTT_Protocol(void)
{

}