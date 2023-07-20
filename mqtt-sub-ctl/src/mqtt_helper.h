#ifndef MQTT_HELPER_H
#define MQTT_HELPER_H

#define MQTT_TOPIC_LENGTH 256
#define MQTT_TOPIC_CAP 16

struct topic {
  char name[MQTT_TOPIC_LENGTH];
  int qos;
};

#endif // MQTT_HELPER_H
