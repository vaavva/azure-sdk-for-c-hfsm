// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#ifdef _MSC_VER
// warning C4201: nonstandard extension used: nameless struct/union
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
#include <paho-mqtt/MQTTClient.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
// Required for Sleep(DWORD)
#include <Windows.h>
#else
// Required for sleep(unsigned int)
#include <unistd.h>
#endif

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/iot/az_iot_provisioning_client.h>
// TODO: can't be internal:
#include <azure/iot/internal/az_iot_sample_config.h>

typedef struct evt_struct
{
  uint8_t type;
  char string[10];
} evt_struct;

#define Q_SIZE 2
#define Q_TYPE evt_struct
#include <azure/iot/internal/az_iot_queue.h>
#include <azure/iot/internal/az_iot_hsm.h>

// Clients
static az_iot_provisioning_client provisioning_client;

static az_iot_hsm test_hsm;

static az_result initial_state(az_iot_hsm* me, az_iot_hsm_event event);
static az_result state_a(az_iot_hsm* me, az_iot_hsm_event event);

static az_result initial_state(az_iot_hsm* me, az_iot_hsm_event event)
{
  az_result ret = AZ_OK;

  switch(event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS("initial_state: event AZ_IOT_HSM_ENTRY");
      // Change states w/o queue:
      ret = az_iot_hsm_transition(me, state_a);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS("initial_state: event AZ_IOT_HSM_EXIT");
      break;

    default:
      ret = az_iot_hsm_super(me, event); // Should assert since this is a top-level HSM.
  }

  return ret;
}

static az_result state_a(az_iot_hsm* me, az_iot_hsm_event event)
{
  az_result ret = AZ_OK;

  switch(event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS("state_a: event AZ_IOT_HSM_ENTRY");
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS("state_a: event AZ_IOT_HSM_EXIT");
      break;

    default:
      ret = az_iot_hsm_super(me, event);
  }

  return ret; 
}

int main()
{
  LOG_SUCCESS("Started.");

  if (az_failed(az_iot_hsm_init(&test_hsm, NULL, initial_state)))
  {
    LOG_ERROR("Failed to init HSM");
  }

  #if Q_TEST
  // TODO: Move to UT
  az_iot_queue q;
  az_iot_queue_init(&q);

  evt_struct e1 = { 1, "Hello 1" };
  evt_struct e2 = { 2, "Hello 2" };
  evt_struct e3 = { 3, "Hello 3" };

  az_iot_queue_enqueue(&q, &e1);
  az_iot_queue_enqueue(&q, &e2);
  az_iot_queue_enqueue(&q, &e3);

  evt_struct* ret;
  ret = az_iot_queue_dequeue(&q);
  ret = az_iot_queue_dequeue(&q);
  ret = az_iot_queue_dequeue(&q);

  az_iot_queue_enqueue(&q, &e1);
  ret = az_iot_queue_dequeue(&q);
  az_iot_queue_enqueue(&q, &e2);
  ret = az_iot_queue_dequeue(&q);
  az_iot_queue_enqueue(&q, &e3);
  ret = az_iot_queue_dequeue(&q);
  az_iot_queue_enqueue(&q, &e1);
  ret = az_iot_queue_dequeue(&q);
  az_iot_queue_enqueue(&q, &e2);
  ret = az_iot_queue_dequeue(&q);
  az_iot_queue_enqueue(&q, &e3);
  ret = az_iot_queue_dequeue(&q);
  #endif



  return 0;
}
