#include "uci_helper.h"
#include "events.h"
#include "mail_helper.h"
#include "mqtt_helper.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <uci.h>

struct uci_context *ctx = NULL;

int uci_init() {
  ctx = uci_alloc_context();
  if (!ctx) {
    syslog(LOG_ERR, "Failed to allocate UCI context");
    return UCI_ERR_MEM;
  }
  return UCI_OK;
}

int uci_load_package(struct uci_package **package, const char *package_path) {
  int rc;
  if ((rc = uci_load(ctx, package_path, package)) != UCI_OK) {
    syslog(LOG_ERR, "Failed to load UCI package: %s", UCI_PACKAGE_PATH);
    return UCI_ERR_IO;
  }
  return UCI_OK;
}

void uci_deinit() { uci_free_context(ctx); }

bool is_duplicate_topic(const struct topic *topics,
                        const struct topic new_topic, int count) {
  for (int i = 0; i < count; i++) {
    if (strcmp(topics[i].name, new_topic.name) == 0)
      return true;
  }

  return false;
}

bool is_duplicate_event(const struct event *events,
                        const struct event new_event, int count) {
  for (int i = 0; i < count; i++) {
    if ((strcmp(events[i].topic.name, new_event.topic.name) == 0) &&
        (strcmp(events[i].key, new_event.key) == 0) &&
        (strcmp(events[i].value_type, new_event.value_type) == 0) &&
        (strcmp(events[i].comparison_type, new_event.comparison_type) == 0) &&
        (strcmp(events[i].value, new_event.value) == 0)) {
      return true;
    }
  }
  return false;
}

int get_topic(struct uci_section *section, struct topic *topic) {
  const char *name_value = uci_lookup_option_string(ctx, section, "name");
  const char *qos_value = uci_lookup_option_string(ctx, section, "qos");

  if (name_value == NULL || qos_value == NULL)
    return UCI_ERR_NOTFOUND;

  strncpy(topic->name, name_value, MQTT_TOPIC_LENGTH);
  topic->name[MQTT_TOPIC_LENGTH - 1] = '\0';

  topic->qos = atoi(qos_value);

  return UCI_OK;
}

int get_all_topics(struct topic *topics) {
  int count = 0;

  struct uci_package *package;
  if (uci_load_package(&package, UCI_PACKAGE_PATH) != UCI_OK)
    return UCI_ERR_IO;

  struct uci_element *i;
  uci_foreach_element(&package->sections, i) {
    if (count == MQTT_TOPIC_CAP) {
      syslog(LOG_INFO, "Reached the maximum topic limit");
      return UCI_OK;
    }

    struct uci_section *curr_section = uci_to_section(i);
    if (strcmp(curr_section->type, "topic") != 0)
      continue;

    struct topic new_topic;

    if (get_topic(curr_section, &new_topic) != UCI_OK)
      continue;
    if (is_duplicate_topic(topics, new_topic, count))
      continue;

    memcpy(&(topics[count]), &new_topic, sizeof(new_topic));
    count++;
  }
  uci_unload(ctx, package);

  return count;
}

int get_event(struct uci_section *section, struct event *event) {
  const struct {
    const char *option_name;
    void *target_field;
    size_t target_field_size;
  } mapping[] = {
      {"topic", &event->topic, sizeof(event->topic)},
      {"key", event->key, sizeof(event->key)},
      {"comparison_type", event->comparison_type,
       sizeof(event->comparison_type)},
      {"value_type", &event->value_type, sizeof(event->value_type)},
      {"value", event->value, sizeof(event->value)},
      {"sender_email", event->sender_email, sizeof(event->sender_email)},
      {"recipient_emails", event->recipient_emails,
       sizeof(event->recipient_emails)},
  };

  for (size_t i = 0; i < sizeof(mapping) / sizeof(mapping[0]); ++i) {
    struct uci_option *o =
        uci_lookup_option(ctx, section, mapping[i].option_name);
    if (o != NULL) {
      if (o->type == UCI_TYPE_STRING) {
        strncpy(mapping[i].target_field, o->v.string,
                mapping[i].target_field_size);
        ((char *)mapping[i].target_field)[mapping[i].target_field_size - 1] =
            '\0';
      } else if (o->type == UCI_TYPE_LIST) {
        struct uci_element *j;
        int count = 0;
        uci_foreach_element(&o->v.list, j) {
          if (count == MAIL_MAX_RECIPIENTS)
            break;
          strncpy(event->recipient_emails[count], j->name,
                  sizeof(event->recipient_emails[0]));
          event->recipient_emails[count][sizeof(event->recipient_emails[0]) -
                                         1] = '\0';
          count++;
        }
        event->recipient_count = count;
      }
    }
  }

  return UCI_OK;
}

int get_all_topic_events(struct event *events, const char *topic) {
  int count = 0;

  struct uci_package *package;
  if (uci_load_package(&package, UCI_PACKAGE_PATH) != UCI_OK)
    return UCI_ERR_IO;

  struct uci_element *i;
  uci_foreach_element(&package->sections, i) {
    if (count == EVENT_CAP) {
      syslog(LOG_INFO, "Reached the maximum event limit");
      return UCI_OK;
    }

    struct uci_section *curr_section = uci_to_section(i);
    if (strcmp(curr_section->type, "event") != 0)
      continue;

    struct event new_event;

    if (get_event(curr_section, &new_event) != UCI_OK)
      continue;

    if (strcmp(new_event.topic.name, topic) != 0)
      continue;

    if (is_duplicate_event(events, new_event, count))
      continue;

    memcpy(&(events[count]), &new_event, sizeof(new_event));
    count++;
  }
  uci_unload(ctx, package);

  return count;
}

int get_sender_info(struct sender_info *sender_info, const char *sender_email) {
  struct uci_package *package;
  int rc = UCI_OK;
  int j = 0;
  bool found = false;

  if ((rc = uci_load_package(&package, "user_groups")) != UCI_OK)
    return rc;

  struct uci_element *i;
  uci_foreach_element(&package->sections, i) {
    struct uci_section *curr_section = uci_to_section(i);
    struct uci_ptr ptr;

    char path[BUFFER_SIZE];
    snprintf(path, sizeof(path), "user_groups.@email[%d].senderemail", j++);

    if (uci_lookup_ptr(ctx, &ptr, path, true) != UCI_OK) {
      syslog(LOG_ERR, "Failed to lookup sender email");
      uci_unload(ctx, package);
      return UCI_ERR_INVAL;
    }

    if (strcmp(ptr.o->v.string, sender_email) != 0)
      continue;

    found = true;

    const char *server = uci_lookup_option_string(ctx, curr_section, "smtp_ip");
    const char *port = uci_lookup_option_string(ctx, curr_section, "smtp_port");
    const char *credentials =
        uci_lookup_option_string(ctx, curr_section, "credentials");
    const char *username =
        uci_lookup_option_string(ctx, curr_section, "username");
    const char *password =
        uci_lookup_option_string(ctx, curr_section, "password");

    if (server == NULL || port == NULL || credentials == NULL ||
        username == NULL || password == NULL) {
      syslog(LOG_ERR, "Missing sender info configuration");
      uci_unload(ctx, package);
      return UCI_ERR_NOTFOUND;
    }

    strncpy(sender_info->server, server, MAIL_FIELD_LENGTH_MAX - 1);
    sender_info->server[MAIL_FIELD_LENGTH_MAX - 1] = '\0';

    sender_info->port = atoi(port);

    sender_info->credentials = atoi(credentials);

    strncpy(sender_info->username, username, MAIL_FIELD_LENGTH_MAX - 1);
    sender_info->username[MAIL_FIELD_LENGTH_MAX - 1] = '\0';

    strncpy(sender_info->password, password, MAIL_FIELD_LENGTH_MAX - 1);
    sender_info->password[MAIL_FIELD_LENGTH_MAX - 1] = '\0';

    strncpy(sender_info->sender_email, sender_email, MAIL_FIELD_LENGTH_MAX - 1);
    sender_info->sender_email[MAIL_FIELD_LENGTH_MAX - 1] = '\0';

    break;
  }

  uci_unload(ctx, package);

  if (!found) {
    syslog(LOG_ERR,
           "Could not find specified sender email account '%s' in user_groups "
           "config",
           sender_email);
    rc = UCI_ERR_NOTFOUND;
  }

  return rc;
}

int find_next_topic_index(struct uci_package *pkg) {
  int index = 0;
  struct uci_element *element;
  uci_foreach_element(&pkg->sections, element) {
    struct uci_section *s = uci_to_section(element);
    const char *type = uci_lookup_option_string(ctx, s, "type");
    if (type && strcmp(type, "topic") == 0) {
      index++;
    }
  }
  return index;
}

int insert_topic_to_mqtt_sub(int client_socket, const char *json_str) {
  struct uci_package *package;
  int rc;

  struct json_object *json_obj = json_tokener_parse(json_str);
  if (!json_obj) {
    syslog(LOG_ERR, "Failed to parse JSON");
    return -1;
  }

  struct topic new_topic;
  memset(&new_topic, 0, sizeof(struct topic));

  json_object_object_foreach(json_obj, key, val) {
    if (strcmp(key, "name") == 0) {
      const char *name = json_object_get_string(val);
      strncpy(new_topic.name, name, MQTT_TOPIC_LENGTH - 1);
      new_topic.name[EVENT_FIELD_LENGTH_MAX - 1] = '\0';
    } else if (strcmp(key, "qos") == 0) {
      new_topic.qos = json_object_get_int(val);
    }
  }

  rc = uci_load_package(&package, "mqtt_sub");
  if (rc != UCI_OK) {
    syslog(LOG_ERR, "Failed to load UCI package");
    return -1;
  }

  char qos_str[2];
  sprintf(qos_str, "%d", new_topic.qos);

  uci_unload(ctx, package);
  return 0;
}

int insert_event_to_mqtt_sub(int client_socket, const char *json_str) {
  struct uci_package *package;
  int rc;

  struct json_object *json_obj = json_tokener_parse(json_str);
  if (!json_obj) {
    syslog(LOG_ERR, "Failed to parse JSON");
    return -1;
  }

  struct event new_event;
  memset(&new_event, 0, sizeof(struct event));

  json_object_object_foreach(json_obj, key, val) {
    if (strcmp(key, "topic") == 0) {
      const char *topic_name = json_object_get_string(val);
      strcpy(new_event.topic.name, topic_name);
    } else if (strcmp(key, "key") == 0) {
      const char *key_val = json_object_get_string(val);
      strncpy(new_event.key, key_val, EVENT_FIELD_LENGTH_MAX - 1);
      new_event.key[EVENT_FIELD_LENGTH_MAX - 1] = '\0';
    } else if (strcmp(key, "value_type") == 0) {
      const char *value_type_val = json_object_get_string(val);
      strncpy(new_event.value_type, value_type_val, EVENT_FIELD_LENGTH_MAX - 1);
      new_event.value_type[EVENT_FIELD_LENGTH_MAX - 1] = '\0';
    } else if (strcmp(key, "comparison_type") == 0) {
      const char *comparison_type_val = json_object_get_string(val);
      strncpy(new_event.comparison_type, comparison_type_val, 2);
      new_event.comparison_type[2] = '\0';
    } else if (strcmp(key, "value") == 0) {
      const char *value_val = json_object_get_string(val);
      strncpy(new_event.value, value_val, 63);
      new_event.value[63] = '\0';
    } else if (strcmp(key, "sender_email") == 0) {
      const char *sender_email_val = json_object_get_string(val);
      strncpy(new_event.sender_email, sender_email_val,
              MAIL_FIELD_LENGTH_MAX - 1);
      new_event.sender_email[MAIL_FIELD_LENGTH_MAX - 1] = '\0';
    } else if (strcmp(key, "recipient_emails") == 0) {
      int array_len = json_object_array_length(val);
      new_event.recipient_count =
          (array_len > MAIL_MAX_RECIPIENTS) ? MAIL_MAX_RECIPIENTS : array_len;

      for (int i = 0; i < new_event.recipient_count; i++) {
        struct json_object *item = json_object_array_get_idx(val, i);
        const char *recipient_email = json_object_get_string(item);
        strncpy(new_event.recipient_emails[i], recipient_email,
                MAIL_FIELD_LENGTH_MAX - 1);
        new_event.recipient_emails[i][MAIL_FIELD_LENGTH_MAX - 1] = '\0';
      }
    }
  }

  rc = uci_load_package(&package, "mqtt_sub");
  if (rc != UCI_OK) {
    syslog(LOG_ERR, "Failed to load UCI package");
    uci_deinit();
    return -1;
  }

  uci_unload(ctx, package);
  return 0;
}
