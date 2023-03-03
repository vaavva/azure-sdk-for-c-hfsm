#ifndef _az_MQTT_H
#define _az_MQTT_H

#include <azure/core/az_span.h>


typedef void (*az_mqtt_inbound_handler)(az_mqtt* mqtt, az_hfsm_event event);

typedef struct
{
  /**
   * The CA Trusted Roots span interpretable by the underlying MQTT implementation.
   */
  az_span certificate_authority_trusted_roots;
} az_mqtt_options;

struct az_mqtt
{
  struct
  {
    az_mqtt_inbound_handler _inbound_handler;
    az_mqtt_options options;
  } _internal;
};

#endif // _az_MQTT_H
