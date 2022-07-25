// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/internal/az_result_internal.h>
#include <azure/core/az_hfsm.h>
#include <azure/core/az_hfsm_pipeline.h>

#include <azure/core/_az_cfg.h>


AZ_NODISCARD az_result az_hfsm_dispatch_process_current(az_hfsm_dispatch_pipeline* h)
{
    
}

AZ_NODISCARD az_result
az_hfsm_dispatch_post_inbound_event(az_hfsm_dispatch_pipeline* h, az_hfsm_event const* event_reference);


AZ_NODISCARD az_result
az_hfsm_dispatch_post_outbound_event(az_hfsm_dispatch_pipeline* h, az_hfsm_event const* event_reference);



#ifdef AZ_HFSM_PIPELINE_SYNC
AZ_NODISCARD az_result az_hfsm_dispatch_process_events(az_hfsm_dispatch_pipeline* h)
{
    for (int i = 0; (i < _az_MAXIMUM_NUMBER_OF_POLICIES) && (h->policies[i] != NULL); i++)
    {
        az_hfsm_event const* inbound_event = h->policies[i]._internal.inbound_mailbox;
        az_hfsm_event const* outbound_event = h->policies[i]._internal.inbound_mailbox;

        if (inbound_event != NULL)
        {
            if (i <= 0)
            {
                return AZ_ERROR_ITEM_NOT_FOUND;
            }

            _az_PRECONDITION_NOT_NULL(h->policies[i-1]);
            policies[i]._internal.inbound_mailbox = NULL;
            az_hfsm_send_event(h->policies[i-1], inbound_event);
        }

        if (outbound_event != NULL)
        {
            if ((i + 1 >= _az_MAXIMUM_NUMBER_OF_POLICIES))
            {
                return AZ_ERROR_ITEM_NOT_FOUND;
            }

            _az_PRECONDITION_NOT_NULL(h->policies[i+1]);
            policies[i]._internal.outbound_mailbox = NULL;
            az_hfsm_send_event(h->policies[i+1], outbound_event);
        }
    }

    return AZ_OK;
}
#endif