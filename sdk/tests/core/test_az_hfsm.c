// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include "az_test_definitions.h"
#include <azure/core/internal/az_hfsm_internal.h>
#include <azure/core/az_result.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>

#include <cmocka.h>

static az_hfsm az_hfsm_test;

// State handlers
static int SRoot(az_hfsm* me, az_hfsm_event event);
static int S01(az_hfsm* me, az_hfsm_event event);
static int S02(az_hfsm* me, az_hfsm_event event);
static int S11(az_hfsm* me, az_hfsm_event event);
static int S12(az_hfsm* me, az_hfsm_event event);
static int S21(az_hfsm* me, az_hfsm_event event);
static int S22(az_hfsm* me, az_hfsm_event event);

// Test HFSM hierarchy structure
static az_hfsm_state_handler az_hfsm_get_parent_test(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if ((child_state == SRoot))
  {
    parent_state = NULL;
  }
  else if ((child_state == S01) || (child_state == S02))
  {
    parent_state = SRoot;
  }
  else if ((child_state == S11) || (child_state == S12))
  {
    parent_state = S01;
  }
  else if ((child_state == S21) || (child_state == S22))
  {
    parent_state = S11;
  }
  else
  {
    // Unknown state.
    parent_state = NULL;
  }

  return parent_state;
}

// Test HFSM specific events
typedef enum
{
  T_INTERNAL_0 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 5),
  T_INTERNAL_1 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 6),
  T_INTERNAL_2 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 7),
  T_SUB_R = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 8),
  T_SUB_0 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 9),
  T_SUB_1 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 10),
  T_SUPER_1 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 11),
  T_SUPER_2 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 12),
  T_PEER_0 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 13),
  T_PEER_1 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 14),
  T_PEER_2 = _az_HFSM_MAKE_EVENT(_az_FACILITY_HFSM, 15)
} test_az_hfsm_event_type;

static int refroot = 0;
static int ref01 = 0;
static int ref02 = 0;
static int ref11 = 0;
static int ref12 = 0;
static int ref21 = 0;
static int ref22 = 0;
static int tinternal0 = 0;
static int tinternal1 = 0;
static int tinternal2 = 0;

//TestHFSM/SRoot Root State
static int SRoot(az_hfsm* me, az_hfsm_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      refroot++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      refroot--;
      break;

    case T_SUB_R:
      ret = az_hfsm_transition_substate(me, SRoot, S01);
      break;

    default:
      assert_false(ret);
  }

  return ret;
}

// TestHFSM/S01
static int S01(az_hfsm* me, az_hfsm_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref01++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref01--;
      break;

    case T_SUB_0:
      ret = az_hfsm_transition_substate(me, S01, S11);
      break;

    case T_PEER_0:
      ret = az_hfsm_transition_peer(me, S01, S02);
      break;

    case T_INTERNAL_0:
      tinternal0++;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/S02
static int S02(az_hfsm* me, az_hfsm_event event)
{
  az_hfsm_state_handler curr_state = me->_internal.current_state; 
  assert_true(curr_state);

  int ret = AZ_OK;
  
  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref02++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref02--;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/S01/S11
static int S11(az_hfsm* me, az_hfsm_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref11++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref11--;
      break;

    case T_SUB_1:
      ret = az_hfsm_transition_substate(me, S11, S21);
      break;

    case T_PEER_1:
      ret = az_hfsm_transition_peer(me, S11, S12);
      break;

    case T_INTERNAL_1:
      tinternal1++;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/S01/S12
static int S12(az_hfsm* me, az_hfsm_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref12++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref12--;
      break;

    case T_SUPER_1:
      ret = az_hfsm_transition_superstate(me, S12, S01);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/S01/S11/S21
static int S21(az_hfsm* me, az_hfsm_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref21++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref21--;
      break;

    case T_PEER_2:
      ret = az_hfsm_transition_peer(me, S21, S22);
      break;

    case T_INTERNAL_2:
      tinternal2++;
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// TestHFSM/S01/S11/S22
static int S22(az_hfsm* me, az_hfsm_event event)
{
  int ret = AZ_OK;

  switch ((int)event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      ref22++;
      break;

    case AZ_HFSM_EVENT_EXIT:
      ref22--;
      break;

    case T_SUPER_2:
      ret = az_hfsm_transition_superstate(me, S22, S11);
      break;

    default:
      ret = AZ_HFSM_RETURN_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

static az_hfsm_event tinternal0_evt = { T_INTERNAL_0, NULL };
static az_hfsm_event tinternal1_evt = { T_INTERNAL_1, NULL };
static az_hfsm_event tinternal2_evt = { T_INTERNAL_2, NULL };

static az_hfsm_event tsubr_evt = { T_SUB_R, NULL };
static az_hfsm_event tsub0_evt = { T_SUB_0, NULL };
static az_hfsm_event tsub1_evt = { T_SUB_1, NULL };

static az_hfsm_event tpeer0_evt = { T_PEER_0, NULL };
static az_hfsm_event tpeer1_evt = { T_PEER_1, NULL };
static az_hfsm_event tpeer2_evt = { T_PEER_2, NULL };

static az_hfsm_event tsuper1_evt = { T_SUPER_1, NULL };
static az_hfsm_event tsuper2_evt = { T_SUPER_2, NULL };

static void test_az_hfsm_stack_internal_transitions(void** state)
{
  (void)state;

  // Init SRoot
  assert_true(az_hfsm_init(&az_hfsm_test, SRoot, az_hfsm_get_parent_test) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == SRoot);
  assert_true(refroot == 1);

  // TSubR S01 -> SRoot
  assert_true(az_hfsm_send_event(&az_hfsm_test, tsubr_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S01);
  assert_true(refroot == 1 && ref01 == 1);

  // TSub0 S01 -> S11
  assert_true(az_hfsm_send_event(&az_hfsm_test, tsub0_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S11);
  assert_true(refroot == 1 && ref01 == 1 && ref11 == 1);

  // TSub1 S21
  assert_true(az_hfsm_send_event(&az_hfsm_test, tsub1_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S21);
  assert_true(refroot == 1 && ref01 == 1 && ref11 == 1 && ref21 == 1);

  //TInternal2 S21
  assert_true(az_hfsm_send_event(&az_hfsm_test, tinternal2_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S21);
  assert_true(tinternal2 == 1 && ref11 == 1 && ref21 == 1);

  // TInternal1 S21
  assert_true(az_hfsm_send_event(&az_hfsm_test, tinternal1_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S21);
  assert_true(tinternal1 == 1 && ref21 == 1);

  // TInternal0 S21
  assert_true(az_hfsm_send_event(&az_hfsm_test, tinternal0_evt) == AZ_OK);
  assert_true(tinternal0 == 1);

  // TPeer2 S21 -> s22
  assert_true(az_hfsm_send_event(&az_hfsm_test, tpeer2_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S22);
  assert_true(refroot == 1 && ref01 == 1 && ref11 == 1 && ref22 == 1);
  assert_true(ref21 == 0);

  // TInternal1 S22
  assert_true(az_hfsm_send_event(&az_hfsm_test, tinternal1_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S22);
  assert_true(tinternal1 == 2 && ref22 == 1);

  // TInternal0 S22
  assert_true(az_hfsm_send_event(&az_hfsm_test, tinternal0_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S22);
  assert_true(tinternal0 == 2 && ref22 == 1);

  // TSuper2 S22 -> S11
  assert_true(az_hfsm_send_event(&az_hfsm_test, tsuper2_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S11);
  assert_true(refroot == 1 && ref01 == 1 && ref11 == 1);
  assert_true(ref21 == 0 && ref22 == 0);

  // TInternal1 S11
  assert_true(az_hfsm_send_event(&az_hfsm_test, tinternal1_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S11);
  assert_true(tinternal1 == 3);

  // TInternal0 S11
  assert_true(az_hfsm_send_event(&az_hfsm_test, tinternal0_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S11);
  assert_true(tinternal0 == 3 && ref11 == 1);

  // TPeer1 S11 -> S12
  assert_true(az_hfsm_send_event(&az_hfsm_test, tpeer1_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S12);
  assert_true(refroot == 1 && ref01 == 1 && ref12 == 1);
  assert_true(ref21 == 0 && ref22 == 0 && ref11 == 0);

  // TInternal0 S12
  assert_true(az_hfsm_send_event(&az_hfsm_test, tinternal0_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S12);
  assert_true(tinternal0 == 4 && ref12 == 1);

  // TSuper1 S12 -> S01
  assert_true(az_hfsm_send_event(&az_hfsm_test, tsuper1_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S01);
  assert_true(refroot == 1 && ref01 == 1);
  assert_true(ref21 == 0 && ref22 == 0 && ref11 == 0 && ref12 == 0);

  // TInternal0 S01
  assert_true(az_hfsm_send_event(&az_hfsm_test, tinternal0_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S01);
  assert_true(tinternal0 == 5 && ref01 == 1);

  //TPeer0 S01 -> S02
  assert_true(az_hfsm_send_event(&az_hfsm_test, tpeer0_evt) == AZ_OK);
  assert_true(az_hfsm_test._internal.current_state == S02);
  assert_true(refroot == 1 && ref02 == 1);
  assert_true(ref21 == 0 && ref22 == 0 && ref11 == 0 && ref12 == 0 && ref01 == 1);

}

int test_az_hfsm()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_hfsm_stack_internal_transitions),
  };
  return cmocka_run_group_tests_name("az_core_hfsm", tests, NULL, NULL);
}
