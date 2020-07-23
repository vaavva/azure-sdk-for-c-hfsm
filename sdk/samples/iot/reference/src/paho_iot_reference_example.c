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
#include <azure/iot/internal/az_iot_hsm.h>
#include <azure/iot/internal/az_iot_queue.h>

// Clients
static az_iot_provisioning_client provisioning_client;

static az_iot_hsm test_hsm;

static az_result idle(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result connected(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result subscribing(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result subscribed(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result disconnecting(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result disconnected(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);

typedef enum
{
  PAHO_IOT_HSM_CONNECTED = _az_IOT_HSM_EVENT(1),
  PAHO_IOT_HSM_SUBSCRIBED = _az_IOT_HSM_EVENT(2),
} paho_iot_hsm_event_type;

// TestHSM/Idle
static az_result idle(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = NULL; // Top-level state.
  }
  
  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    case PAHO_IOT_HSM_CONNECTED:
      LOG_SUCCESS("%s: event PAHO_IOT_HSM_CONNECTED", __func__);
      ret = az_iot_hsm_transition(me, idle, connected);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR("%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// TestHSM/Connected
static az_result connected(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = NULL; // Top-level state.
  }

  switch (event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_ENTRY", __func__);
      // TODO: is the subsribing state really needed?
      ret = az_iot_hsm_transition_inner(me, connected, subscribing);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    case AZ_IOT_HSM_ERROR:
      LOG_ERROR("%s: event AZ_IOT_HSM_ERROR", __func__);
      // Handled for all inner states (i.e. subscribing, subscribed).
      ret = az_iot_hsm_transition(me, connected, disconnecting);

      // TODO: move to UT :Test transition from super-class to inner.
      //ret = az_iot_hsm_transition_inner(me, connected, subscribing);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR("%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// TestHSM/Connected/Subscribing
static az_result subscribing(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = (void*)connected;
  }

  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    case PAHO_IOT_HSM_SUBSCRIBED:
      LOG_SUCCESS("%s: event PAHO_IOT_HSM_SUBSCRIBED", __func__);
      ret = az_iot_hsm_transition(me, subscribing, subscribed);
      break;

    default:
      // Handle using super-state.
      LOG_SUCCESS("%s: handling event using super-state", __func__);
      ret = connected(me, event, NULL);
  }

  return ret;
}

// TestHSM/Connected/Subscribed
static az_result subscribed(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = (void*)connected;
  }

  switch (event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    default:
      // Handle using super-state.
      LOG_SUCCESS("%s: handling event using super-state", __func__);
      ret = connected(me, event, NULL);
  }

  return ret;
}

// TestHSM/Disconnecting
static az_result disconnecting(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = NULL; // Top-level state.
  }

  (void)me;

  switch (event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR("%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// TestHSM/Disconnected
static az_result disconnected(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = NULL; // Top-level state.
  }

  (void)me;

  switch (event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS("%s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR("%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

int main()
{
  LOG_SUCCESS("--> Started.");

  if (az_failed(az_iot_hsm_init(&test_hsm, idle)))
  {
    LOG_ERROR("Failed to init HSM");
  }

  LOG_SUCCESS("--> Connected event.");

  // Memory must be allocated for the duration of the transition:
  az_iot_hsm_event connected_evt = { PAHO_IOT_HSM_CONNECTED, NULL };
  if (az_failed(az_iot_hsm_post_sync(&test_hsm, connected_evt)))
  {
    LOG_ERROR("Failed to post connected_evt");
  }

  LOG_SUCCESS("--> Subscribed event.");

  az_iot_hsm_event subscribed_evt = { PAHO_IOT_HSM_SUBSCRIBED, NULL };
  if (az_failed(az_iot_hsm_post_sync(&test_hsm, subscribed_evt)))
  {
    LOG_ERROR("Failed to post subscribed_evt");
  }

  LOG_SUCCESS("--> Simulating ERROR event.");
  az_iot_hsm_event error_evt = { AZ_IOT_HSM_ERROR, NULL };
  if (az_failed(az_iot_hsm_post_sync(&test_hsm, error_evt)))
  {
    LOG_ERROR("Failed to post subscribed_evt");
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
