
typedef 
{
  // Last message and topic lifetime management.
  MQTTClient_message *last_message;
  char* last_topic;
} az_mqtt_impl_data;

typedef MQTTClient az_mqtt_impl;
typedef char az_mqtt_impl_options; //unused
