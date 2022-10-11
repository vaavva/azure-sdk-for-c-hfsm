/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file event_clone.h
 * @brief HFSM event clone functionality
 * 
 */

#ifndef _az_HFSM_EVENT_CLONE_H
#define _az_HFSM_EVENT_CLONE_H

#include <azure/core/az_hfsm.h>

az_result az_hfsm_event_clone(az_hfsm_event original, az_hfsm_event* out_clone);
az_result az_hfsm_event_destroy(az_hfsm_event* event);

#endif //_az_HFSM_EVENT_CLONE_H
