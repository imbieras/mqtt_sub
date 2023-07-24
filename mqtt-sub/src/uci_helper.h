#ifndef UCI_HELPER_H
#define UCI_HELPER_H

#include "events.h"
#include "mqtt_helper.h"
#include <uci.h>

#define UCI_PACKAGE_PATH "mqtt_sub"

int uci_init();
void uci_deinit();
size_t get_all_topics(struct topic *topics);
size_t get_all_topic_events(struct event *events, const char *topic);
int get_sender_info(struct sender_info *sender_info, const char *sender_email);
int insert_topic_to_mqtt_sub(int client_socket, const char *json_str);
int insert_event_to_mqtt_sub(int client_socket, const char *json_str);

#endif // UCI_HELPER_H
