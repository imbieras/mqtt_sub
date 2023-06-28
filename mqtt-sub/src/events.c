#include "events.h"
#include "helper.h"
#include "mail_helper.h"
#include "uci_helper.h"
#include <curl/curl.h>
#include <json-c/json.h>
#include <json-c/json_object.h>
#include <stdio.h>
#include <string.h>
#include <sys/syslog.h>
#include <syslog.h>

int get_comparison_type(const char *operator_str) {
  if (strcmp(operator_str, "<") == 0) {
    return CMP_LESS;
  } else if (strcmp(operator_str, ">") == 0) {
    return CMP_MORE;
  } else if (strcmp(operator_str, "==") == 0) {
    return CMP_EQ;
  } else if (strcmp(operator_str, "!=") == 0) {
    return CMP_NOTEQ;
  } else if (strcmp(operator_str, ">=") == 0) {
    return CMP_MOREQ;
  } else if (strcmp(operator_str, "<=") == 0) {
    return CMP_LESEQ;
  }

  // Default case if operator_str does not match any known operator
  return CMP_EQ;
}

int compare_numbers(int val1, int val2, int comparison_type) {
  switch (comparison_type) {
  case CMP_LESS:
    return val1 < val2;
  case CMP_MORE:
    return val1 > val2;
  case CMP_EQ:
    return val1 == val2;
  case CMP_NOTEQ:
    return val1 != val2;
  case CMP_MOREQ:
    return val1 >= val2;
  case CMP_LESEQ:
    return val1 <= val2;
  }
  return -1;
}

int compare_strings(const char *val1, const char *val2, int comparison_type) {
  int cmp = strcmp(val1, val2);
  switch (comparison_type) {
  case CMP_EQ:
    return cmp == 0;
  case CMP_NOTEQ:
    return cmp != 0;
  }
  return -1;
}

int compare_values(const char *val1, const char *val2, const char *value_type,
                   int comparison_type) {
  if (strncmp(value_type, "decimal", sizeof("decimal")) == 0) {
    int int_val1 = atoi(val1);
    int int_val2 = atoi(val2);
    return compare_numbers(int_val1, int_val2, comparison_type);
  } else if (strncmp(value_type, "string", sizeof("string")) == 0) {
    return compare_strings(val1, val2, comparison_type);
  }
  return -1;
}

json_object *find_by_key(json_object *obj, char *key) {
  if (!obj || json_object_get_type(obj) != json_type_object || !key)
    return NULL;

  struct json_object_iterator current = json_object_iter_begin(obj);
  struct json_object_iterator end = json_object_iter_end(obj);

  while (!json_object_iter_equal(&current, &end)) {
    if (strcmp(json_object_iter_peek_name(&current), key) == 0) {
      return json_object_get(json_object_iter_peek_value(&current));
    }
    json_object_iter_next(&current);
  }

  return NULL;
}

int check_and_send_events(const char *payload, const char *topic) {
  int events_sent = 0;

  struct event events[EVENT_CAP];
  json_object *obj = json_tokener_parse(payload);
  if (obj == NULL) {
    syslog(LOG_ERR, "Failed to parse JSON payload");
    return events_sent;
  }

  json_object *data = find_by_key(obj, "data");
  if (data == NULL) {
    syslog(LOG_ERR, "Failed to find 'data' key in JSON payload");
    json_object_put(obj);
    return events_sent;
  }

  int count = get_all_topic_events(events, topic);

  for (int i = 0; i < count; i++) {
    json_object *key_value = find_by_key(data, events[i].key);
    if (key_value == NULL) {
      syslog(LOG_ERR, "Failed to find key '%s' in JSON payload", events[i].key);
      continue;
    }

    const char *key_value_str = json_object_get_string(key_value);
    if (key_value_str == NULL) {
      syslog(LOG_ERR, "Failed to get value for key '%s' in JSON payload",
             events[i].key);
      continue;
    }

    int ret =
        compare_values(key_value_str, events[i].value, events[i].value_type,
                       get_comparison_type(events[i].comparison_type));
    if (ret > 0) {

      char message[BUFFER_SIZE];
      snprintf(message, sizeof(message),
               "Event triggered:\r\nTopic: %s\r\nKey: %s\r\nComparison "
               "value: "
               "%s\r\nReal value: %s\r\nComparison "
               "type: %s\r\n",
               events[i].topic.name, events[i].key, events[i].value,
               key_value_str, events[i].comparison_type);
      syslog(LOG_INFO, "%s", message);

      int email_res =
          send_email(events[i].sender_email, events[i].recipient_emails,
                     events[i].recipient_count, events[i].topic.name, message);
      if (email_res == CURLE_OK) {
        syslog(LOG_INFO,
               "Successfully sent email for event: topic='%s', key='%s'",
               events[i].topic.name, events[i].key);
        events_sent++;
      }
    } else if (ret < 0) {
      syslog(LOG_ERR,
             "Failed to compare values for event: topic='%s', key='%s'",
             events[i].topic.name, events[i].key);
    }
  }

  json_object_put(obj);

  return events_sent;
}
