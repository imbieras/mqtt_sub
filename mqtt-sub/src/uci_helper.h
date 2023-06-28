#ifndef UCI_HELPER_H
#define UCI_HELPER_H

#include "events.h"
#include "mqtt_helper.h"
#include <uci.h>

#define UCI_PACKAGE_PATH "mqtt_sub"

int uci_init();
int uci_load_package(struct uci_package **package, const char *package_path);
void uci_deinit();
bool is_duplicate_topic(const struct topic *topics,
                        const struct topic new_topic, int count);
bool is_duplicate_event(const struct event *events,
                        const struct event new_event, int count);
int get_topic(struct uci_section *section, struct topic *topic);
int get_all_topics(struct topic *topics);
int get_event(struct uci_section *section, struct event *event);
int get_all_topic_events(struct event *events, const char *topic);
int get_sender_info(struct sender_info *sender_info, const char *sender_email);

#endif // UCI_HELPER_H
