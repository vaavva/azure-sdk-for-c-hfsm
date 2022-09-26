// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license
// information.

#ifndef PROV_TRANSPORT_H
#define PROV_TRANSPORT_H

#ifdef __cplusplus
extern "C"
{
#include <cstdint>
#include <cstdlib>
#else
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#endif /* __cplusplus */

  struct PROV_DEVICE_TRANSPORT_PROVIDER_TAG;
  typedef struct PROV_DEVICE_TRANSPORT_PROVIDER_TAG PROV_DEVICE_TRANSPORT_PROVIDER;

  typedef void* PROV_DEVICE_TRANSPORT_HANDLE;

  typedef enum
  {
    TRANSPORT_HSM_TYPE_X509,
    TRANSPORT_HSM_TYPE_SYMM_KEY
  } TRANSPORT_HSM_TYPE;

  typedef enum
  {
    PROV_DEVICE_TRANSPORT_RESULT_OK,
    PROV_DEVICE_TRANSPORT_RESULT_UNAUTHORIZED,
    PROV_DEVICE_TRANSPORT_RESULT_ERROR
  } PROV_DEVICE_TRANSPORT_RESULT;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PROV_TRANSPORT
