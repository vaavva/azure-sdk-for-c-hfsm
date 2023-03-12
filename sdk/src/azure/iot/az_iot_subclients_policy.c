// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_mqtt.h>
#include <azure/core/az_platform.h>
#include <azure/core/az_result.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/iot/internal/az_iot_subclients_policy.h>

#include <azure/core/_az_cfg.h>

static az_result _az_iot_subclients_process_outbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_iot_subclients_policy* subclients_policy = (_az_iot_subclients_policy*)policy;
  _az_iot_subclient* last = subclients_policy->subclients;
  while (last->next != NULL)
  {
    az_event_policy_send_outbound_event(last->policy, event);
    last = last->next;
  }
}

static az_result _az_iot_subclients_process_inbound_event(
    az_event_policy* policy,
    az_event const event)
{
  _az_iot_subclients_policy* subclients_policy = (_az_iot_subclients_policy*)policy;
  _az_iot_subclient* last = subclients_policy->subclients;
  while (last->next != NULL)
  {
    az_event_policy_send_inbound_event(last->policy, event);
    last = last->next;
  }
}

AZ_NODISCARD az_result _az_iot_subclients_policy_init(
    _az_iot_subclients_policy* subclients_policy,
    az_event_policy* outbound_policy,
    az_event_policy* inbound_policy)
{
    subclients_policy->policy.outbound_policy = outbound_policy;
    subclients_policy->policy.inbound_policy = inbound_policy;
    subclients_policy->policy.outbound_handler = _az_iot_subclients_process_outbound_event;
    subclients_policy->policy.inbound_handler = _az_iot_subclients_process_inbound_event;

    subclients_policy->subclients = NULL;
}

AZ_NODISCARD az_result _az_iot_subclients_policy_add_client(
    _az_iot_subclients_policy* subclients_policy,
    _az_iot_subclient* subclient)
{
    _az_PRECONDITION_NOT_NULL(subclients_policy);
    _az_PRECONDITION_NOT_NULL(subclient);

    subclient->next = NULL;
    _az_iot_subclient* last = subclients_policy->subclients;
    while (last->next != NULL)
    {
      last = last->next;
    }

    last->next = subclient;

    return AZ_OK;
}
