/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file az_iot_provisioning_hfsm.c
 * @brief HFSM for Azure IoT Provisioning Operations.
 *
 * @details Implements connectivity and fault handling for an IoT Hub Client
 */

#include <stdint.h>

#include <azure/core/az_hfsm.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/core/internal/az_result_internal.h>

#include <azure/iot/internal/az_iot_provisioning_hfsm.h>

#include <azure/core/_az_cfg.h>

static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event);

static az_hfsm_state_handler _get_parent(az_hfsm_state_handler child_state)
{
  az_hfsm_state_handler parent_state;

  if (child_state == root)
  {
    parent_state = NULL;
  }
  else
  {
    // Unknown state.
    az_platform_critical_error();
    parent_state = NULL;
  }

  return parent_state;
}

AZ_NODISCARD az_hfsm_iot_provisioning_policy_options
az_hfsm_iot_provisioning_policy_options_default()
{
  return (az_hfsm_iot_provisioning_policy_options){ ._reserved = 0 };
}

AZ_NODISCARD az_result az_hfsm_iot_provisioning_policy_initialize(
    az_hfsm_iot_provisioning_policy* policy,
    az_hfsm_pipeline* pipeline,
    az_hfsm_policy* inbound_policy,
    az_hfsm_policy* outbound_policy,
    az_iot_provisioning_client* provisioning_client,
    az_hfsm_iot_provisioning_policy_options const* options)
{
  policy->_internal.options
      = options == NULL ? az_hfsm_iot_provisioning_policy_options_default() : *options;

  policy->_internal.policy.outbound = outbound_policy;
  policy->_internal.policy.inbound = inbound_policy;

  policy->_internal.pipeline = pipeline;

  policy->_internal.provisioning_client = provisioning_client;

  _az_RETURN_IF_FAILED(az_hfsm_init((az_hfsm*)policy, root, _get_parent));
  //az_hfsm_transition_substate((az_hfsm*)policy, root, idle);

  return AZ_OK;
}

static az_hfsm_return_type root(az_hfsm* me, az_hfsm_event event)
{
  int32_t ret = AZ_HFSM_RETURN_HANDLED;
  (void)me;

  if (_az_LOG_SHOULD_WRITE(event.type))
  {
    _az_LOG_WRITE(event.type, AZ_SPAN_FROM_STR("az_iot_provisioning/root"));
  }

  switch (event.type)
  {
    case AZ_HFSM_EVENT_ENTRY:
      // No-op.
      break;

    case AZ_HFSM_EVENT_EXIT:
      // Exitting root state is not permitted. Flow through default:
    default:
      if (_az_LOG_SHOULD_WRITE(AZ_HFSM_EVENT_EXIT))
      {
        _az_LOG_WRITE(AZ_HFSM_EVENT_EXIT, AZ_SPAN_FROM_STR("az_iot_provisioning/root: PANIC!"));
      }

      az_platform_critical_error();
      break;
  }

  return ret;
}
