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

int subscribe_topic(struct mosquitto *mosq, struct topic topic);
int subscribe_all_topics(struct mosquitto *mosq);
void on_connect(struct mosquitto *mosq, void *obj, int reason_code);
void on_message(struct mosquitto *mosq, void *obj,
                const struct mosquitto_message *msg);
int mqtt_init_login(struct mosquitto *mosq, struct arguments arguments);
int mqtt_init_tls(struct mosquitto *mosq, struct arguments arguments);
int mqtt_init(struct mosquitto **mosq, struct arguments arguments);
int mqtt_loop(struct mosquitto *mosq);
int mqtt_attempt_reconnect(struct mosquitto *mosq);
int mqtt_main(struct arguments arguments);

#endif // MQTT_HELPER_H
