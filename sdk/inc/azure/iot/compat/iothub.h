// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/** @file iothub.h
*    @brief Global initialization and deinitialization routines for all IoT Hub client operations.
*
*/

#ifndef IOTHUB_H
#define IOTHUB_H

#include <azure/core/az_platform.h>

#ifdef __cplusplus
extern "C"
{
#else
#endif
    /**
    * @brief    IoTHubClient_Init Initializes the IoT Hub Client System.
    *
    * @remarks  
    *           This must be called before using any functionality from the IoT Hub
    *           client library, including the device provisioning client.
    *           
    *           @c IoTHubClient_Init should be called once per process, not per-thread.
    *
    * @return   int zero upon success, any other value upon failure.
    */
    int IoTHub_Init();

    /**
    * @brief    IoTHubClient_Deinit Frees resources initialized in the IoTHubClient_Init function call.
    *
    * @remarks
    *           This should be called when using IoT Hub client library, including
    *           the device provisioning client.
    *
    *           This function should be called once per process, not per-thread.
    *
    * @warning
    *           Close all IoT Hub and provisioning client handles prior to invoking this.
    *
    */
    void IoTHub_Deinit();


    AZ_INLINE void ThreadAPI_Sleep(unsigned int milliseconds)
    {
        az_result ret = az_platform_sleep_msec(milliseconds);
    }


    /*Codes_SRS_CRT_ABSTRACTIONS_99_038: [mallocAndstrcpy_s shall allocate memory for destination buffer to fit the string in the source parameter.]*/
    int mallocAndStrcpy_s(char** destination, const char* source);
    
#ifdef __cplusplus
}
#endif

#endif /* IOTHUB_H */
