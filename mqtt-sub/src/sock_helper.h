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

struct server_data {
  int server_socket;
  struct data *subscriber_data;
};

void send_response(int client_socket, const char *response);
void return_all_topics(int client_socket, struct data *subscriber_data);
void return_events_for_topic(int client_socket, struct data *subscriber_data,
                             const char *topic_name);
void handle_controller_request(int client_socket, struct data *subscriber_data);
void *handle_connection(void *arg);
void init_server_socket(struct server_data *server);
void deinit_server_socket(struct server_data *server);

#endif // SOCK_HELPER_H
