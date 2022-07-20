#include <stdlib.h>
#include <stdio.h>
#include "mosquitto.h"

// Mosquitto Lib documentation is available at: https://mosquitto.org/api/files/mosquitto-h.html

volatile bool running = true;

/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
  /* Print out the connection result. mosquitto_connack_string() produces an
   * appropriate string for MQTT v3.x clients, the equivalent for MQTT v5.0
   * clients is mosquitto_reason_string().
   */
  printf("on_connect: %s\n", mosquitto_connack_string(reason_code));
  if(reason_code != 0){
    /* If the connection fails for any reason, we don't want to keep on
     * retrying in this example, so disconnect. Without this, the client
     * will attempt to reconnect. */
    mosquitto_disconnect(mosq);
  }

  /* You may wish to set a flag here to indicate to your application that the
   * client is now connected. */
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
  printf("MOSQ: DISCONNECT reason=%d\n", rc);
  running = false;
}

/* Callback called when the client knows to the best of its abilities that a
 * PUBLISH has been successfully sent. For QoS 0 this means the message has
 * been completely written to the operating system. For QoS 1 this means we
 * have received a PUBACK from the broker. For QoS 2 this means we have
 * received a PUBCOMP from the broker. */
void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
  printf("MOSQ: Message with mid %d has been published.\n", mid);
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
  printf("MOSQ: Subscribed with mid %d; %d topics.\n", mid, qos_count);
  for(int i = 0; i < qos_count; i++)
  {
    printf("MOSQ: \t QoS %d\n", granted_qos[i]);
  }
}

void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid)
{
  printf("MOSQ: Unsubscribing using message with mid %d.\n", mid);
}

void on_log(struct mosquitto *mosq, void *obj, int level, const char *str)
{
  char* log_level;

  switch (level)
  {
    case MOSQ_LOG_INFO:
      log_level = "INFO";
      break;
    case MOSQ_LOG_NOTICE:
      log_level = "NOTI";
      break;
    case MOSQ_LOG_WARNING:
      log_level = "WARN";
      break;
    case MOSQ_LOG_ERR:
      log_level = "ERR ";
      break;
    case MOSQ_LOG_DEBUG:
      log_level = "DBUG";
      break;
    case MOSQ_LOG_SUBSCRIBE:
      log_level = "SUB ";
      break;
    case MOSQ_LOG_UNSUBSCRIBE:
      log_level = "USUB";
      break;
    case MOSQ_LOG_WEBSOCKETS:
      log_level = "WSCK";
      break;
    default:
      log_level = "UNKN";
  }
  printf("MOSQ [%s] %s\n", log_level, str);
}

int main(int argc, char *argv[])
{
  struct mosquitto *mosq;
  int rc;

  /* Required before calling other mosquitto functions */
  mosquitto_lib_init();

  /* Create a new client instance.
   * id = NULL -> ask the broker to generate a client id for us
   * clean session = true -> the broker should remove old sessions when we connect
   * obj = NULL -> we aren't passing any of our private data for callbacks
   */
  mosq = mosquitto_new("dev1-ecc", true, NULL);
  if(mosq == NULL){
    fprintf(stderr, "Error: Out of memory.\n");
    return 1;
  }

  /* Configure callbacks. This should be done before connecting ideally. */
  mosquitto_log_callback_set(mosq, on_log);

  mosquitto_connect_callback_set(mosq, on_connect);
  mosquitto_disconnect_callback_set(mosq, on_disconnect);
  mosquitto_publish_callback_set(mosq, on_publish);
  mosquitto_subscribe_callback_set(mosq, on_subscribe);
  mosquitto_unsubscribe_callback_set(mosq, on_unsubscribe);
  
  rc = mosquitto_tls_set(
    mosq,
    "S:\\test\\rsa_baltimore_ca.pem",
    NULL, //"S:\\cert\\RootCAs",
    "S:\\test\\dev1-ecc_cert.pem",
    "S:\\test\\dev1-ecc_key.pem",
    NULL);
  if (rc != MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy(mosq);
    fprintf(stderr, "TLS Config Error: %s\n", mosquitto_strerror(rc));
    return 1;
  }

  rc = mosquitto_username_pw_set(mosq, "crispop-iothub1.azure-devices.net/dev1-ecc/?api-version=2020-09-30&DeviceClientType=azsdk-c%2F1.4.0-beta.1", "");
  if (rc != MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy(mosq);
    fprintf(stderr, "User/pass Config Error: %s\n", mosquitto_strerror(rc));
    return 1;
  }

  rc = mosquitto_connect_async(mosq, "crispop-iothub1.azure-devices.net", 8883, 60);
  if(rc != MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy(mosq);
    fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
    return 1;
  }

  // Mosquitto BUG: sleep required when connect async used on Windows.
  Sleep(500);

  /* Run the network loop in a background thread, this call returns quickly. */
  rc = mosquitto_loop_start(mosq);
  if(rc != MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy(mosq);
    fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
    return 1;
  }

  /* At this point the client is connected to the network socket, but may not
   * have completed CONNECT/CONNACK.
   * It is fairly safe to start queuing messages at this point, but if you
   * want to be really sure you should wait until after a successful call to
   * the connect callback.
   * In this case we know it is 1 second before we start publishing.
   */
  while(running){
    //publish_sensor_data(mosq);
  }

  mosquitto_lib_cleanup();
  return 0;
}
