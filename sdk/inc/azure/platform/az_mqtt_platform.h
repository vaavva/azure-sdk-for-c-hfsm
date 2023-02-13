// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

/**
 * @file 
 * 
 */

#if defined(TRANSPORT_MOSQUITTO)
    #include <azure/platform/az_mqtt_mosquitto.h>
#elif defined(TRANSPORT_PAHO)
    #include <azure/platform/az_mqtt_paho.h>
#else
    #include <azure/platform/az_mqtt_no_transport.h>
#endif
