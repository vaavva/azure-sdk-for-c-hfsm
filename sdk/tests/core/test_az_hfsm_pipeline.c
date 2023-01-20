// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/internal/az_hfsm_pipeline_internal.h>
#include <azure/core/internal/az_hfsm_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/az_result.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

static az_hfsm_pipeline az_hfsm_pipeline_test;

static az_hfsm_policy P01; // Outbound
static az_hfsm_policy P02;
static az_hfsm_policy P03; // Inbound

typedef enum
{
  P_OUTBOUND_0 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 5),
  P_INBOUND_0 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 6),
  S_INBOUND_0 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 7),
  S_INBOUND_1 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 8),
  S_INBOUND_2 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 9),
  S_INBOUND_3 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 10),
  S_OUTBOUND_0 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 11),
  S_OUTBOUND_1 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 12)
} test_az_hfsm_pipeline_event_type;

static az_hfsm_event poutbound0_evt = { P_OUTBOUND_0, NULL };
static az_hfsm_event pinbound0_evt = { P_INBOUND_0, NULL };
static az_hfsm_event sinbound0_evt = { S_INBOUND_0, NULL };
static az_hfsm_event sinbound1_evt = { S_INBOUND_1, NULL };
static az_hfsm_event sinbound2_evt = { S_INBOUND_2, NULL };
static az_hfsm_event sinbound3_evt = { S_INBOUND_3, NULL };
static az_hfsm_event soutbound0_evt = { S_OUTBOUND_0, NULL };
static az_hfsm_event soutbound1_evt = { S_OUTBOUND_1, NULL };

static int refp01root = 0;
static int refp02root = 0;
static int refp03root = 0;
static int timeout0 = 0;
static int timeout1 = 0;
#ifdef TRANSPORT_MQTT_SYNC
static int loopout0 = 0;
static int loopin0 = 0;
#endif
static int poutbound0 = 0;
static int pinbound0 = 0;
static int sinbound0 = 0;
static int sinbound1 = 0;
static int sinbound2 = 0;
static int sinbound3 = 0;
static int soutbound0 = 0;
static int soutbound1 = 0;

static az_result P01Root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  (void)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      refp01root++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      refp01root--;
      break;

    case AZ_HFSM_EVENT_TIMEOUT:
      timeout0++;
      if (timeout0 > 1)
      {
        // To test timeout failure.
        ret = AZ_ERROR_ARG;
      }
      break;

    #ifdef TRANSPORT_MQTT_SYNC
    case AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP:
      loopout0++;
      break;
    #endif

    case P_OUTBOUND_0:
      poutbound0++;
      break;

    case S_INBOUND_0:
      sinbound0++;
      ret = az_hfsm_pipeline_send_inbound_event((az_hfsm_policy*)me, sinbound1_evt);
      break;

    case S_INBOUND_2:
      sinbound2++;
      ret = az_hfsm_pipeline_send_inbound_event((az_hfsm_policy*)me, sinbound3_evt);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result P02Root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  (void)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      refp02root++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      refp02root--;
      break;

    case AZ_HFSM_EVENT_ERROR:
      sinbound3++;
      break;

    case S_INBOUND_1:
      sinbound1++;
      break;

    case S_INBOUND_3:
      sinbound3++;
      ret = AZ_ERROR_ARG;
      break;

    case S_OUTBOUND_1:
      soutbound1++;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_result P03Root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_OK;
  (void)me;

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      refp03root++; 
      break;

    case AZ_HFSM_EVENT_EXIT:
      refp03root--;
      break;

    #ifdef TRANSPORT_MQTT_SYNC
    case AZ_HFSM_PIPELINE_EVENT_PROCESS_LOOP:
      loopin0++;
      break;
    #endif

    case AZ_HFSM_EVENT_ERROR:
      _az_PRECONDITION_NOT_NULL(event.data);
      az_hfsm_event_data_error* test_error = (az_hfsm_event_data_error*) event.data;
      if (test_error->error_type == AZ_ERROR_ARG)
      {
        timeout1++;
      } else
      {
        ret = AZ_ERROR_NOT_IMPLEMENTED;
      }
      break;

    case P_INBOUND_0:
      pinbound0++;
      break;

    case S_OUTBOUND_0:
      soutbound0++;
      ret = az_hfsm_pipeline_send_outbound_event((az_hfsm_policy*)me, soutbound1_evt);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
      break;
  }

  return ret;
}

static az_hfsm_state_handler p01_get_parent_test(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if ((child_state == P01Root))
  {
    parent_state = NULL;
  } else 
  {
    parent_state = NULL;
  }

  return parent_state;
}

static az_hfsm_state_handler p02_get_parent_test(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if ((child_state == P02Root))
  {
    parent_state = NULL;
  } else 
  {
    parent_state = NULL;
  }

  return parent_state;
}

static az_hfsm_state_handler p03_get_parent_test(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if ((child_state == P03Root))
  {
    parent_state = NULL;
  } else 
  {
    parent_state = NULL;
  }

  return parent_state;
}

static void test_az_hfsm_pipeline_init(void** state)
{
  (void)state;

  // Init P01 policy
  assert_int_equal(az_hfsm_init((az_hfsm*) &P01, P01Root, p01_get_parent_test), AZ_OK);
  assert_true(P01.hfsm._internal.current_state == P01Root);
  assert_true(refp01root == 1);

  // Init P02 policy
  assert_int_equal(az_hfsm_init((az_hfsm*) &P02, P02Root, p02_get_parent_test), AZ_OK);
  assert_true(P02.hfsm._internal.current_state == P02Root);
  assert_true(refp02root == 1);

  // Init P03 policy
  assert_int_equal(az_hfsm_init((az_hfsm*) &P03, P03Root, p03_get_parent_test), AZ_OK);
  assert_true(P03.hfsm._internal.current_state == P03Root);
  assert_true(refp03root == 1);

  P01.inbound = &P02;
  P02.outbound = &P01;
  P02.inbound = &P03;
  P03.outbound = &P02;

  assert_int_equal(az_hfsm_pipeline_init(&az_hfsm_pipeline_test, &P01, &P03), AZ_OK);
}

static void test_az_hfsm_pipeline_post_outbound(void** state)
{
  (void)state;

  assert_int_equal(az_hfsm_pipeline_post_outbound_event(&az_hfsm_pipeline_test, poutbound0_evt),
   AZ_OK);
  assert_true(poutbound0 == 1);
}

static void test_az_hfsm_pipeline_post_inbound(void** state)
{
  (void)state;

  assert_int_equal(az_hfsm_pipeline_post_inbound_event(&az_hfsm_pipeline_test, pinbound0_evt),
   AZ_OK);
  assert_true(pinbound0 == 1);
}

static void test_az_hfsm_pipeline_send_inbound(void** state)
{
  (void)state;
  
  assert_int_equal(az_hfsm_pipeline_post_outbound_event(&az_hfsm_pipeline_test, sinbound0_evt),
   AZ_OK);
  assert_true(sinbound0 == 1);
  assert_true(sinbound1 == 1);
}

static void test_az_hfsm_pipeline_send_inbound_failure(void** state)
{
  (void)state;

  assert_int_equal(az_hfsm_pipeline_post_outbound_event(&az_hfsm_pipeline_test, sinbound2_evt),
   AZ_OK);
  assert_true(sinbound2 == 1);
  assert_true(sinbound3 == 2);
}

static void test_az_hsfm_pipeline_send_outbound(void** state)
{
  (void)state;

  assert_int_equal(az_hfsm_pipeline_post_inbound_event(&az_hfsm_pipeline_test, soutbound0_evt),
   AZ_OK);
  assert_true(soutbound0 == 1);
  assert_true(soutbound1 == 1);
}

static void test_az_hfsm_pipeline_timer_create_cb_success(void** state)
{
  (void)state;
  az_hfsm_pipeline_timer test_timer;
  timeout0 = 0; // Reset timeout counter
  
  assert_int_equal(az_hfsm_pipeline_timer_create(&az_hfsm_pipeline_test, &test_timer), AZ_OK);
  assert_int_equal(az_platform_timer_start(&(test_timer.platform_timer), 0), AZ_OK);
  test_timer.platform_timer._internal.callback(test_timer.platform_timer._internal.sdk_data);
  assert_true(timeout0 == 1);
}

static void test_az_hfsm_pipeline_timer_create_cb_failure(void** state)
{
  (void)state;
  az_hfsm_pipeline_timer test_timer;
  timeout0 = 1; // Will cause failure on timeout

  assert_int_equal(az_hfsm_pipeline_timer_create(&az_hfsm_pipeline_test, &test_timer), AZ_OK);
  assert_int_equal(az_platform_timer_start(&(test_timer.platform_timer), 0), AZ_OK);
  test_timer.platform_timer._internal.callback(test_timer.platform_timer._internal.sdk_data);
  assert_true(timeout0 == 2);
  assert_true(timeout1 == 1);

}

#ifdef TRANSPORT_MQTT_SYNC
static void test_az_hfsm_pipeline_sync_process_loop(void** state)
{
  (void)state;

  assert_int_equal(az_hfsm_pipeline_sync_process_loop(&az_hfsm_pipeline_test), AZ_OK);
  assert_true(loopout0 == 1);
  assert_true(loopin0 == 1);
}
#endif

int test_az_hfsm_pipeline()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_hfsm_pipeline_init),
    cmocka_unit_test(test_az_hfsm_pipeline_post_outbound),
    cmocka_unit_test(test_az_hfsm_pipeline_post_inbound),
    cmocka_unit_test(test_az_hfsm_pipeline_send_inbound),
    cmocka_unit_test(test_az_hfsm_pipeline_send_inbound_failure),
    cmocka_unit_test(test_az_hsfm_pipeline_send_outbound),
    cmocka_unit_test(test_az_hfsm_pipeline_timer_create_cb_success),
    cmocka_unit_test(test_az_hfsm_pipeline_timer_create_cb_failure),
    #ifdef TRANSPORT_MQTT_SYNC
    cmocka_unit_test(test_az_hfsm_pipeline_sync_process_loop),
    #endif
  };
  return cmocka_run_group_tests_name("az_core_hfsm_pipeline", tests, NULL, NULL);
}
