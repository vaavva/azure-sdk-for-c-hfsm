// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/az_platform.h>
#include <azure/core/internal/az_precondition_internal.h>

// Two macros below are not used in the code below, it is windows.h that consumes them.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _CRT_RAND_S
#include <stdlib.h>
#include <windows.h>

#include <azure/core/_az_cfg.h>

AZ_NODISCARD az_result az_platform_clock_msec(int64_t* out_clock_msec)
{
  _az_PRECONDITION_NOT_NULL(out_clock_msec);
  *out_clock_msec = GetTickCount64();
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_sleep_msec(int32_t milliseconds)
{
  Sleep(milliseconds);
  return AZ_OK;
}

AZ_NODISCARD az_result az_platform_get_random(int32_t* out_random)
{
  unsigned int n;
  az_result ret; 
  _az_PRECONDITION_NOT_NULL(out_random);

  errno_t err = rand_s(&n);
   
  if (err != 0)
  {
    ret = AZ_ERROR_ARG;
  }
  else
  {
    *out_random = (int32_t)n;
    ret = AZ_OK;
  }

  return ret;
}
