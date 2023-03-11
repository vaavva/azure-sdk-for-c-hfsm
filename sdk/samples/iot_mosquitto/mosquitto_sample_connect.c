#include <stdlib.h>
#include <stdio.h>
#include "mosquitto.h"

// Mosquitto Lib documentation is available at: https://mosquitto.org/api/files/mosquitto-h.html

// Path to a PEM file containing the server trusted CA.
#define MOSQUITTO_X509_TRUST_PEM_FILE_PATH "[ENTER TRUST FILE PATH]"
// Path to a PEM file containing only the device certificate.
#define MOSQUITTO_X509_CERT_ONLY_PEM_FILE_PATH "[ENTER CERT FILE PATH]"
// Path to a PEM file containing only the device key.
#define MOSQUITTO_X509_KEY_PEM_FILE_PATH "[ENTER KEY FILE PATH]"
// IoT Hub hostname
#define MOSQUITTO_IOT_HUB_HOSTNAME "[ENTER IOT HUB HOSTNAME]"
// IoT Hub device name
#define MOSQUITTO_IOT_HUB_DEVICENAME "[ENTER IOT HUB DEVICE NAME]"
// Define Mosquitto username
#define MOSQUITTO_USERNAME \
  MOSQUITTO_IOT_HUB_HOSTNAME \
  "/" \
  MOSQUITTO_IOT_HUB_DEVICENAME \
  "/?api-version=2020-09-30&DeviceClientType=azsdk-c%2F1.4.0-beta.1"

volatile bool running = true;

void on_connect(struct mosquitto *mosq, void *obj, int reason_code);
void on_disconnect(struct mosquitto *mosq, void *obj, int rc);
void on_publish(struct mosquitto *mosq, void *obj, int mid);
void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos);
void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid);
void on_log(struct mosquitto *mosq, void *obj, int level, const char *str);

/* Callback called when the client receives a CONNACK message from the broker. */
void on_connect(struct mosquitto *mosq, void *obj, int reason_code)
{
  (void)obj;

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
  (void)mosq; (void)obj, (void)rc;
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
  (void)mosq; (void)obj, (void)mid;
  printf("MOSQ: Message with mid %d has been published.\n", mid);
}

void on_subscribe(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
  (void)mosq; (void)obj; (void)mid; (void)qos_count; (void)granted_qos;

  printf("MOSQ: Subscribed with mid %d; %d topics.\n", mid, qos_count);
  for(int i = 0; i < qos_count; i++)
  {
    printf("MOSQ: \t QoS %d\n", granted_qos[i]);
  }
}

void on_unsubscribe(struct mosquitto *mosq, void *obj, int mid)
{
  (void)mosq; (void)obj; (void)mid;

  printf("MOSQ: Unsubscribing using message with mid %d.\n", mid);
}

void on_log(struct mosquitto *mosq, void *obj, int level, const char *str)
{
  (void)mosq; (void)obj;

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
  (void)argc; (void)argv;

  struct mosquitto *mosq;
  int rc;

  /* Required before calling other mosquitto functions */
  mosquitto_lib_init();

  /* Create a new client instance.
   * id = NULL -> ask the broker to generate a client id for us
   * clean session = true -> the broker should remove old sessions when we connect
   * obj = NULL -> we aren't passing any of our private data for callbacks
   */
  mosq = mosquitto_new(MOSQUITTO_IOT_HUB_DEVICENAME, true, NULL);
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
    MOSQUITTO_X509_TRUST_PEM_FILE_PATH,
    NULL,
    MOSQUITTO_X509_CERT_ONLY_PEM_FILE_PATH,
    MOSQUITTO_X509_KEY_PEM_FILE_PATH,
    NULL);
  if (rc != MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy(mosq);
    fprintf(stderr, "TLS Config Error: %s\n", mosquitto_strerror(rc));
    return 1;
  }

  rc = mosquitto_username_pw_set(mosq, MOSQUITTO_USERNAME, "");
  if (rc != MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy(mosq);
    fprintf(stderr, "User/pass Config Error: %s\n", mosquitto_strerror(rc));
    return 1;
  }

  rc = mosquitto_connect_async(mosq, MOSQUITTO_IOT_HUB_HOSTNAME, 8883, 60);
  if(rc != MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy(mosq);
    fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
    return 1;
  }

  // HFSM_TODO : Mosquitto BUG: sleep required when connect async used on Windows.
  //Sleep(500);

  /* Run the network loop in a background thread, this call returns quickly. */
  rc = mosquitto_loop_start(mosq);
  if(rc != MOSQ_ERR_SUCCESS)
  {
    mosquitto_destroy(mosq);
    fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
    return 1;
  }

  mosquitto_lib_cleanup();
  return 0;
}
