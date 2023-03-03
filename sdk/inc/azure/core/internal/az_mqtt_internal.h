#ifndef _az_MQTT_H
#define _az_MQTT_H

#include <azure/core/az_span.h>
#include <azure/core/az_event.h>

typedef void (*az_mqtt_inbound_handler)(az_mqtt* mqtt, az_event event);

typedef struct
{
  /**
   * The CA Trusted Roots span interpretable by the underlying MQTT implementation.
   */
  az_span certificate_authority_trusted_roots;
} az_mqtt_options;

typedef struct
{
  az_mqtt_inbound_handler _inbound_handler;
  az_mqtt_options options;
} az_mqtt_common;

#endif // _az_MQTT_H
