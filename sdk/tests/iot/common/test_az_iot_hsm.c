// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include "test_az_iot_hsm.h"
#include <azure/iot/internal/az_iot_common_internal.h>
#include <azure/iot/internal/az_iot_hsm.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <cmocka.h>

typedef struct evt_struct
{
  uint8_t type;
  char string[10];
} evt_struct;

#define Q_SIZE 2
#define Q_TYPE evt_struct
#include <azure/iot/internal/az_iot_queue.h>

static az_iot_hsm test_hsm;

static az_result idle(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)());
static az_result connected(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)());
static az_result subscribing(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)());
static az_result subscribed(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)());
static az_result disconnecting(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)());

typedef enum
{
  PAHO_IOT_HSM_CONNECTED = _az_IOT_HSM_EVENT(1),
  PAHO_IOT_HSM_SUBSCRIBED = _az_IOT_HSM_EVENT(2),
} paho_iot_hsm_event_type;

//
// Logging
//
#define LOG_ERROR(...)                                                  \
  do                                                                               \
  {                                                                                \
    (void)fprintf(stderr, "ERROR:\t\t%s:%s():%d: ", __FILE__, __func__, __LINE__); \
    (void)fprintf(stderr, __VA_ARGS__);                                            \
    (void)fprintf(stderr, "\n");                                                   \
    fflush(stdout);                                                                \
    fflush(stderr);                                                                \
  } while (0)

#define LOG_SUCCESS(...) \
  do                                \
  {                                 \
    (void)printf("SUCCESS:\t");     \
    (void)printf(__VA_ARGS__);      \
    (void)printf("\n");             \
  } while (0)

// TestHSM/Idle
static az_result idle(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)())
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
static az_result connected(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)())
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
static az_result subscribing(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)())
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = connected;
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
static az_result subscribed(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)())
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = connected;
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
static az_result disconnecting(az_iot_hsm* me, az_iot_hsm_event event, az_result(** super_state)())
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

static void test_az_iot_hsm_succeeds(void **state)
{
  (void)state;

  LOG_SUCCESS("--> Started.");

  if (az_result_failed(az_iot_hsm_init(&test_hsm, idle)))
  {
    LOG_ERROR("Failed to init HSM");
  }

  LOG_SUCCESS("--> Connected event.");

  // Memory must be allocated for the duration of the transition:
  az_iot_hsm_event connected_evt = { PAHO_IOT_HSM_CONNECTED, NULL };
  if (az_result_failed(az_iot_hsm_post_sync(&test_hsm, connected_evt)))
  {
    LOG_ERROR("Failed to post connected_evt");
  }

  LOG_SUCCESS("--> Subscribed event.");

  az_iot_hsm_event subscribed_evt = { PAHO_IOT_HSM_SUBSCRIBED, NULL };
  if (az_result_failed(az_iot_hsm_post_sync(&test_hsm, subscribed_evt)))
  {
    LOG_ERROR("Failed to post subscribed_evt");
  }

  LOG_SUCCESS("--> Simulating ERROR event.");
  az_iot_hsm_event error_evt = { AZ_IOT_HSM_ERROR, NULL };
  if (az_result_failed(az_iot_hsm_post_sync(&test_hsm, error_evt)))
  {
    LOG_ERROR("Failed to post subscribed_evt");
  }
}

int test_az_iot_hsm()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_iot_hsm_succeeds),
  };
  return cmocka_run_group_tests_name("az_iot_hsm", tests, NULL, NULL);
}
