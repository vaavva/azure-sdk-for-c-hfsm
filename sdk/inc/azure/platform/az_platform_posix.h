// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <azure/core/internal/az_platform_timer_internal.h>

typedef struct
{
  struct
  {
    az_platform_timer_callback callback;
    void* sdk_data;

    // POSIX specific
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec trigger;
  } _internal;
} az_platform_timer;

typedef pthread_mutex_t az_platform_mutex;
