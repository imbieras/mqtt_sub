#ifndef SOCK_HELPER_H
#define SOCK_HELPER_H

#include "events.h"
#include "mqtt_helper.h"

struct data {
  struct topic topics[MQTT_TOPIC_CAP];
  int topic_count;
  struct event events[MQTT_TOPIC_CAP][EVENT_CAP];
  int event_counts[MQTT_TOPIC_CAP];
};

void send_request_and_receive_response(const char *request);

#endif // SOCK_HELPER_H
