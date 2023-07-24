#ifndef MQTT_HELPER_H
#define MQTT_HELPER_H

#include "helper.h"
#include <mosquitto.h>

#define MQTT_TOPIC_LENGTH 256
#define MQTT_TOPIC_CAP 16
#define MQTT_RECONNECT_MAX_TRIES 5

struct topic {
  char name[MQTT_TOPIC_LENGTH];
  int qos;
};

int cache_topic_events();
size_t subscribe_all_topics();
void on_connect(struct mosquitto *mosq, void *obj, int reason_code);
void on_message(struct mosquitto *mosq, void *obj,
                const struct mosquitto_message *msg);
int mqtt_main(struct arguments arguments);

#endif // MQTT_HELPER_H
