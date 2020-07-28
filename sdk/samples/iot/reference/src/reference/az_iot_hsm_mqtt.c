// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <azure/core/az_result.h>
#include <azure/core/az_span.h>
#include <azure/core/internal/az_log_internal.h>
#include <azure/core/internal/az_precondition_internal.h>
#include <azure/iot/internal/az_iot_hsm.h>
#include <azure/iot/internal/az_iot_hsm_mqtt.h>
#include <azure/iot/internal/az_iot_sample_log.h>
#define Q_SIZE 10
#define Q_TYPE az_iot_hsm_event
#include <azure/iot/internal/az_iot_queue.h>

#include <azure/core/_az_cfg.h>

#define LOG_HSM_NAME "AZ_IOT_MQTT"

az_iot_hsm az_iot_hsm_mqtt;

static az_result disconnected(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result connecting(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result connected(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result idle(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result pub_sending(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result sub_sending(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result disconnecting(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);
static az_result criticalerror(az_iot_hsm* me, az_iot_hsm_event event, void** super_state);

// MqttClient/Disconnected
static az_result disconnected(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = NULL; // Top-level state.
  }

  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_ENTRY", __func__);
      // TODO: PAL initialize

      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    case CONNECT_REQ:
      LOG_SUCCESS(LOG_HSM_NAME "%s: event CONNECT_REQ", __func__);
      ret = az_iot_hsm_transition(me, disconnected, connecting);
      break;

    case DISCONNECT_REQ:
      LOG_SUCCESS(LOG_HSM_NAME "%s: event DISCONNECT_REQ", __func__);
      // TODO: ^DISCONNECT_RSP(success)
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR(LOG_HSM_NAME "%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// MqttClient/Connecting
static az_result connecting(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = NULL; // Top-level state.
  }

  // TODO: remove
  (void)me;

  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR(LOG_HSM_NAME "%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// MqttClient/Connected
static az_result connected(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = NULL; // Top-level state.
  }

  // TODO: remove
  (void)me;

  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR(LOG_HSM_NAME "%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// MqttClient/Connected/Idle
static az_result idle(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = (void*)connected; // Top-level state.
  }

  // TODO: remove
  (void)me;

  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR(LOG_HSM_NAME "%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// MqttClient/Connected/PUB_Sending
static az_result pub_sending(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = (void*)connected; // Top-level state.
  }

  // TODO: remove
  (void)me;

  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR(LOG_HSM_NAME "%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// MqttClient/Connected/SUB_Sending
static az_result sub_sending(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = (void*)connected; // Top-level state.
  }

  // TODO: remove
  (void)me;

  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR(LOG_HSM_NAME "%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// MqttClient/Disconnecting
static az_result disconnecting(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = NULL; // Top-level state.
  }

  // TODO: remove
  (void)me;

  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_ENTRY", __func__);
      break;

    case AZ_IOT_HSM_EXIT:
      LOG_SUCCESS(LOG_HSM_NAME " %s: event AZ_IOT_HSM_EXIT", __func__);
      break;

    default:
      // TOP level - ignore unknown events.
      LOG_ERROR(LOG_HSM_NAME "%s: dropped unknown event: 0x%x", __func__, event.type);
      ret = AZ_OK;
  }

  return ret;
}

// MqttClient/CriticalError
static az_result criticalerror(az_iot_hsm* me, az_iot_hsm_event event, void** super_state)
{
  az_result ret = AZ_OK;
  if (super_state)
  {
    *super_state = NULL; // Top-level state.
  }

  (void*)me;

  switch ((int)event.type)
  {
    case AZ_IOT_HSM_ENTRY:
      LOG_ERROR(LOG_HSM_NAME "%s: Critical Failure", __func__);
      while (1)
      {
      } // Infinite loop.
      break;
  }

  return ret;
}
