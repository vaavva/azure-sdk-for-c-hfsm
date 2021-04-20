// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include <stdlib.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

#include "test_az_iot_common.h"
#include "test_az_iot_queue.h"
#include "test_az_iot_hsm_stack.h"

int main()
{
  int result = 0;

  result += test_az_iot_common();
  result += test_az_iot_queue();
  result += test_az_iot_hsm_stack();

  return result;
}
