#ifndef SOCK_HELPER_H
#define SOCK_HELPER_H

#include "events.h"
#include "mqtt_helper.h"

struct data {
  struct topic topics[MQTT_TOPIC_CAP];
  size_t topic_count;
  struct event events[MQTT_TOPIC_CAP][EVENT_CAP];
  size_t event_counts[MQTT_TOPIC_CAP];
};

struct server_data {
  int server_socket;
  struct data *subscriber_data;
};

void send_response(int client_socket, const char *response);
void handle_controller_request(int client_socket, struct data *subscriber_data);
void *handle_connection(void *arg);
void init_server_socket(struct server_data *server);
void deinit_server_socket(struct server_data *server);

#endif // SOCK_HELPER_H
