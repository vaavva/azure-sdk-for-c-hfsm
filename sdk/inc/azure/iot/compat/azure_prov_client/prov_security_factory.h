// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifndef PROV_SECURITY_FACTORY_H
#define PROV_SECURITY_FACTORY_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#endif /* __cplusplus */

typedef enum
{
    SECURE_DEVICE_TYPE_UNKNOWN,
    SECURE_DEVICE_TYPE_TPM,
    SECURE_DEVICE_TYPE_X509,
    SECURE_DEVICE_TYPE_HTTP_EDGE,
    SECURE_DEVICE_TYPE_SYMMETRIC_KEY
} SECURE_DEVICE_TYPE;

int prov_dev_security_init(SECURE_DEVICE_TYPE hsm_type);
void prov_dev_security_deinit();
SECURE_DEVICE_TYPE prov_dev_security_get_type();

// Symmetric key information
int prov_dev_set_symmetric_key_info(const char* registration_name, const char* symmetric_key);
const char* prov_dev_get_symmetric_key();
const char* prov_dev_get_symm_registration_name();
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PROV_SECURITY_FACTORY_H
