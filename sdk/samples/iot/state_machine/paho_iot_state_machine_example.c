// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/internal/az_iot_hsm.h>

// TODO: can't be internal:
#include <azure/iot/internal/az_iot_sample_config.h>

#include <azure/iot/internal/az_iot_hsm_mqtt.h>

  /* TODO items:
   * Application should allow 2 sets of credentials for the same ID.
   */

int main()
{
  int rc;
  az_span endpoint;
  az_span id_scope;
  az_span registration_id;

  if (az_failed(rc = read_environment_variables(&endpoint, &id_scope, &registration_id)))
  {
    LOG_ERROR("Failed to read evironment variables: az_result return code 0x%04x.", rc);
    exit(rc);
  }

  

   

  return 0;
}
