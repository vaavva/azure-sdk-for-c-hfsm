/* Copyright (c) Microsoft Corporation. All rights reserved. */
/* SPDX-License-Identifier: MIT */

/**
 * @file queue.h
 * @brief Simple queue template.
 * 
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <azure/core/az_precondition.h>
#include <azure/core/az_result.h>

/* Simple queue parameters - these must be defined by the compilation unit prior to including the template. */
#ifndef Q_SIZE
#define Q_SIZE 5
#endif

#ifndef Q_TYPE
#include <stdint.h>
#define Q_TYPE int32_t
#endif

#ifndef Q_IDX_TYPE
#define Q_IDX_TYPE int32_t
#endif

typedef struct queue
{
  Q_TYPE data[Q_SIZE];
  Q_IDX_TYPE start_idx;
  Q_IDX_TYPE end_idx;
  Q_IDX_TYPE count;
} queue;

static void queue_init(queue* q)
{
  _az_PRECONDITION_NOT_NULL(q);

  q->count = 0;
  q->start_idx = 0;
  q->end_idx = 0;
}

static az_result queue_enqueue(queue* q, Q_TYPE element)
{
  _az_PRECONDITION_NOT_NULL(q);

  az_result ret;

  if (q->count < Q_SIZE)
  {
    q->data[q->end_idx] = element;
    q->end_idx = (q->end_idx + 1) % Q_SIZE;
    q->count++;

    ret = AZ_OK;
  }
  else
  {
    ret = AZ_ERROR_NOT_ENOUGH_SPACE;
  }

  return ret;
}

static az_result queue_dequeue(queue* q, Q_TYPE* element)
{
  _az_PRECONDITION_NOT_NULL(q);

  az_result ret;

  if (q->count > 0)
  {
    *element = q->data[q->start_idx];
    q->start_idx = (q->start_idx + 1) % Q_SIZE;
    q->count--;

    ret = AZ_OK;
  }
  else
  {
    ret = AZ_ERROR_ITEM_NOT_FOUND;
  }

  return ret;
}

static az_result queue_peek(queue* q, Q_TYPE* element)
{
  _az_PRECONDITION_NOT_NULL(q);

  az_result ret;

  if (q->count > 0)
  {
    *element = q->data[q->start_idx];
    ret = AZ_OK;
  }
  else
  {
    ret = AZ_ERROR_ITEM_NOT_FOUND;
  }

  return ret;
}

#endif //!QUEUE_H
