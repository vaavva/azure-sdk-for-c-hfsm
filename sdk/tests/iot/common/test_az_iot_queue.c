// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT
#include "test_az_iot_queue.h"
#include <azure/az_core.h>
#include <azure/iot/internal/az_iot_common_internal.h>

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

#include <cmocka.h>

typedef struct test_queue_struct
{
  uint8_t type;
  char string[10];
} test_queue_struct;

// Must be defined before including queue.h.
#define Q_SIZE 2
#define Q_TYPE test_queue_struct
#include <azure/iot/internal/az_iot_queue.h>

test_queue_struct e1 = { 1, "Hello 1" };
test_queue_struct e2 = { 2, "Hello 2" };
test_queue_struct e3 = { 3, "Hello 3" };

static void test_az_queue_dequeue_succeeds(void** state)
{
  (void)state;

  az_iot_queue q;
  az_iot_queue_init(&q);

  az_iot_queue_enqueue(&q, &e1);
  az_iot_queue_enqueue(&q, &e2);

  test_queue_struct* ret;
  ret = az_iot_queue_dequeue(&q);
  assert_int_equal(e1.type, ret->type);
  ret = az_iot_queue_dequeue(&q);
  assert_int_equal(e2.type, ret->type);

  az_iot_queue_enqueue(&q, &e3);
  ret = az_iot_queue_dequeue(&q);
  assert_int_equal(e3.type, ret->type);

  az_iot_queue_enqueue(&q, &e2);
  az_iot_queue_enqueue(&q, &e1);
  ret = az_iot_queue_dequeue(&q);
  assert_int_equal(e2.type, ret->type);
  ret = az_iot_queue_dequeue(&q);
  assert_int_equal(e1.type, ret->type);
}

static void test_az_queue_enqueue_overflow_fails(void** state)
{
  (void)state;

  az_iot_queue q;
  az_iot_queue_init(&q);

  assert_true(az_iot_queue_enqueue(&q, &e1));
  assert_true(az_iot_queue_enqueue(&q, &e2));
  assert_false(az_iot_queue_enqueue(&q, &e3));
  assert_false(az_iot_queue_enqueue(&q, &e1));

  test_queue_struct* ret;
  ret = az_iot_queue_dequeue(&q);
  assert_int_equal(e1.type, ret->type);

  ret = az_iot_queue_dequeue(&q);
  assert_int_equal(e2.type, ret->type);
}

static void test_az_queue_dequeue_underflow_fails(void** state)
{
  (void)state;

  az_iot_queue q;
  az_iot_queue_init(&q);

  assert_true(az_iot_queue_enqueue(&q, &e1));
  assert_true(az_iot_queue_enqueue(&q, &e2));

  test_queue_struct* ret;
  ret = az_iot_queue_dequeue(&q);
  assert_int_equal(e1.type, ret->type);

  ret = az_iot_queue_dequeue(&q);
  assert_int_equal(e2.type, ret->type);

  ret = az_iot_queue_dequeue(&q);
  assert_ptr_equal(NULL, ret);
}

int test_az_iot_queue()
{
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(test_az_queue_dequeue_succeeds),
    cmocka_unit_test(test_az_queue_enqueue_overflow_fails),
    cmocka_unit_test(test_az_queue_dequeue_underflow_fails),
  };
  return cmocka_run_group_tests_name("az_iot_queue", tests, NULL, NULL);
}
