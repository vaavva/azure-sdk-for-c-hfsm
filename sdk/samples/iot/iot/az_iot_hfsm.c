
#include <stdint.h>
#include <assert.h>
#include <azure/az_core.h>

#include <az_iot_hfsm.h>

static hfsm az_iot_hfsm;

#define USE_PROVISIONING  // Comment out if Azure IoT Provisioning is not used.

static int azure_iot(hfsm* me, hfsm_event event);
#ifdef USE_PROVISIONING
static int provisioning(hfsm* me, hfsm_event event);
static int retry_provisioning(hfsm* me, hfsm_event event);
#endif
static int hub(hfsm* me, hfsm_event event);
static int retry_hub(hfsm* me, hfsm_event event);

// Hardcoded AzureIoT hierarchy structure
static state_handler azure_iot_hfsm_get_parent(state_handler child_state)
{
   state_handler parent_state;

  if ((child_state == azure_iot))
  {
    parent_state = NULL;
  }
  else if (
#ifdef USE_PROVISIONING
      (child_state == provisioning) || (child_state == retry_provisioning)
#endif
      (child_state == hub) || (child_state == retry_hub) 
    )
  {
    parent_state = azure_iot;
  }
  else {
    // Unknown state.
    assert(false);
    parent_state = NULL;
  }

  return parent_state;
}

// AzureIoT
static int azure_iot(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      ret = hfsm_transition_substate(me, azure_iot, provisioning);
      break;

    case HFSM_EXIT:
      break;

    case ERROR:
    default:
      // Critical error: (memory corruption likely) reboot device.
      assert(0);
      break;
  }

  return ret;
}

// AzureIoT/Provisioning
static int provisioning(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      break;

    case HFSM_EXIT:
      break;

    default:
      printf("Unknown event %d", event.type);
      assert(0);
  }

  return ret;
}

// AzureIoT/S01/S11
static int retry_provisioning(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      ref11++;
      ret = hfsm_transition_substate(me, S11, S21);
      break;

    case HFSM_EXIT:
      ref11--;
      break;

    case T_PEER_1:
      ret = hfsm_transition_peer(me, S11, S12);
      break;

    case T_INTERNAL_1:
      tinternal1++;
      break;

    default:
      ret = RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoT/S01/S12
static int S12(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      ref12++;
      break;

    case HFSM_EXIT:
      ref12--;
      break;

    case T_SUPER_1:
      ret = hfsm_transition_superstate(me, S12, S01);
      break;

    default:
      ret = RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoT/S01/S11/S21
static int S21(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      ref21++;
      break;

    case HFSM_EXIT:
      ref21--;
      break;

    case T_PEER_2:
      ret = hfsm_transition_peer(me, S21, S22);
      break;

    case T_INTERNAL_2:
      tinternal2++;
      break;

    default:
      ret = RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

// AzureIoT/S01/S11/S22
static int S22(hfsm* me, hfsm_event event)
{
  int ret = 0;

  switch ((int)event.type)
  {
    case HFSM_ENTRY:
      ref22++;
      break;

    case HFSM_EXIT:
      ref22--;
      break;

    case T_SUPER_2:
      ret = hfsm_transition_superstate(me, S22, S11);
      break;

    default:
      ret = RET_HANDLE_BY_SUPERSTATE;
  }

  return ret;
}

static hfsm_event tinternal0_evt = { T_INTERNAL_0, NULL };
static hfsm_event tinternal1_evt = { T_INTERNAL_1, NULL };
static hfsm_event tinternal2_evt = { T_INTERNAL_2, NULL };

static hfsm_event tpeer0_evt = { T_PEER_0, NULL };
static hfsm_event tpeer1_evt = { T_PEER_1, NULL };
static hfsm_event tpeer2_evt = { T_PEER_2, NULL };

static hfsm_event tsuper1_evt = { T_SUPER_1, NULL };
static hfsm_event tsuper2_evt = { T_SUPER_2, NULL };

int test_hfsm_stack_internal_transitions()
{
  // Init, TSub0, TSub1, TSub2
  ASSERT_TRUE(!hfsm_init(&test_hfsm, S01, test_hfsm_get_parent));
  ASSERT_TRUE(test_hfsm.current_state == S21);
  ASSERT_TRUE(ref01 == 1 && ref11 == 1 && ref21 == 1);

  // TInternal2
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tinternal2_evt));
  ASSERT_TRUE(tinternal2 == 1);

  // TInternal1 S21
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tinternal1_evt));
  ASSERT_TRUE(tinternal2 == 1 && tinternal1 == 1);

  // TInternal0 S21
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tinternal0_evt));
  ASSERT_TRUE(tinternal2 == 1 && tinternal1 == 1 && tinternal0 == 1);

  // TPeer2 S21
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer2_evt));
  ASSERT_TRUE(test_hfsm.current_state == S22);
  ASSERT_TRUE(ref01 == 1 && ref11 == 1 && ref22 == 1);
  ASSERT_TRUE(ref21 == 0);

  // TInternal1 S22
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tinternal1_evt));
  ASSERT_TRUE(tinternal1 == 2);

  // TInternal0 S22
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tinternal0_evt));
  ASSERT_TRUE(tinternal1 == 2 && tinternal0 == 2);
  ASSERT_TRUE(tinternal2 == 1);

  // TSuper2 S22
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tsuper2_evt));
  ASSERT_TRUE(test_hfsm.current_state == S11);
  ASSERT_TRUE(ref01 == 1 && ref11 == 1);
  ASSERT_TRUE(ref21 == 0 && ref22 == 0);

  // TInternal1 S11
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tinternal1_evt));
  ASSERT_TRUE(tinternal1 == 3);

  // TInternal0 S11
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tinternal0_evt));
  ASSERT_TRUE(tinternal1 == 3 && tinternal0 == 3);

  // TPeer1 S11
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer1_evt));
  ASSERT_TRUE(test_hfsm.current_state == S12);
  ASSERT_TRUE(ref01 == 1 && ref12 == 1);
  ASSERT_TRUE(ref11 == 0 && ref21 == 0 && ref22 == 0);

  // TInternal0 S12
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tinternal0_evt));
  ASSERT_TRUE(tinternal0 == 4);

  // TSuper1 S12
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tsuper1_evt));
  ASSERT_TRUE(test_hfsm.current_state == S01);
  ASSERT_TRUE(ref01 == 1);
  ASSERT_TRUE(ref11 == 0 && ref12 == 0 && ref21 == 0 && ref22 == 0);

  // TInternal0 S01
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tinternal0_evt));
  ASSERT_TRUE(tinternal0 == 5);

  return 0;
}

int init_peer_transitions_state_s21()
{
  ref01 = 0;
  ref02 = 0;
  ref11 = 0;
  ref12 = 0;
  ref21 = 0;
  ref22 = 0;

    // Init, TSub0, TSub1, TSub2
  ASSERT_TRUE(!hfsm_init(&test_hfsm, S01, test_hfsm_get_parent));
  ASSERT_TRUE(test_hfsm.current_state == S21);
  ASSERT_TRUE(ref01 == 1 && ref11 == 1 && ref21 == 1);
  return 0;
}

int test_hfsm_stack_peer_transitions()
{
  // TPeer1 S21
  if(init_peer_transitions_state_s21()) return 1;
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer1_evt));
  ASSERT_TRUE(test_hfsm.current_state == S12);
  ASSERT_TRUE(ref01 == 1 && ref12 == 1);
  ASSERT_TRUE(ref11 == 0 && ref21 == 0 && ref22 == 0);

  // TPeer1 S22
  if(init_peer_transitions_state_s21()) return 1;
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer2_evt));
  ASSERT_TRUE(test_hfsm.current_state == S22);
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer1_evt));
  ASSERT_TRUE(test_hfsm.current_state == S12);
  ASSERT_TRUE(ref01 == 1 && ref12 == 1);
  ASSERT_TRUE(ref11 == 0 && ref21 == 0 && ref22 == 0);

  // TPeer0 S21
  if(init_peer_transitions_state_s21()) return 1;
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer0_evt));
  ASSERT_TRUE(test_hfsm.current_state == S02);
  ASSERT_TRUE(ref02 == 1);
  ASSERT_TRUE(ref01 == 0 && ref11 == 0 && ref12 == 0 && ref21 == 0 && ref22 == 0);

  // TPeer0 S22
  if(init_peer_transitions_state_s21()) return 1;
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer2_evt));
  ASSERT_TRUE(test_hfsm.current_state == S22);
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer0_evt));
  ASSERT_TRUE(test_hfsm.current_state == S02);
  ASSERT_TRUE(ref02 == 1);
  ASSERT_TRUE(ref01 == 0 && ref11 == 0 && ref12 == 0 && ref21 == 0 && ref22 == 0);

  // TPeer0 S11
  if(init_peer_transitions_state_s21()) return 1;
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer2_evt));
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tsuper2_evt));
  ASSERT_TRUE(test_hfsm.current_state == S11);
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer0_evt));
  ASSERT_TRUE(test_hfsm.current_state == S02);
  ASSERT_TRUE(ref02 == 1);
  ASSERT_TRUE(ref01 == 0 && ref11 == 0 && ref12 == 0 && ref21 == 0 && ref22 == 0);

  // TPeer0 S12
  if(init_peer_transitions_state_s21()) return 1;
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer2_evt));
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tsuper2_evt));
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer1_evt));
  ASSERT_TRUE(test_hfsm.current_state == S12);
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer0_evt));
  ASSERT_TRUE(test_hfsm.current_state == S02);
  ASSERT_TRUE(ref02 == 1);
  ASSERT_TRUE(ref01 == 0 && ref11 == 0 && ref12 == 0 && ref21 == 0 && ref22 == 0);

  // TPeer0 S01
  if(init_peer_transitions_state_s21()) return 1;
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer2_evt));
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tsuper2_evt));
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer1_evt));
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tsuper1_evt));
  ASSERT_TRUE(test_hfsm.current_state == S01);
  ASSERT_TRUE(!hfsm_post_event(&test_hfsm, tpeer0_evt));
  ASSERT_TRUE(test_hfsm.current_state == S02);
  ASSERT_TRUE(ref02 == 1);
  ASSERT_TRUE(ref01 == 0 && ref11 == 0 && ref12 == 0 && ref21 == 0 && ref22 == 0);

  return 0;
}

int main()
{

  int ret = 0;
  ret += test_hfsm_stack_internal_transitions();
  ret += test_hfsm_stack_peer_transitions();

  printf("Test %s\n", (ret > 0) ? "FAILED" : "PASSED");

  return ret;
}
