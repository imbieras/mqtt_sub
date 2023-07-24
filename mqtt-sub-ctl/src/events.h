#ifndef EVENTS_H
#define EVENTS_H

#include "mail_helper.h"
#include "mqtt_helper.h"
#include <stdlib.h>

#define EVENT_FIELD_LENGTH_MAX 256
#define EVENT_CAP 16

struct event {
  struct topic topic;
  char key[EVENT_FIELD_LENGTH_MAX];
  char value_type[EVENT_FIELD_LENGTH_MAX];
  char comparison_type[3];
  char value[64];
  char sender_email[MAIL_FIELD_LENGTH_MAX];
  char recipient_emails[MAIL_MAX_RECIPIENTS][MAIL_FIELD_LENGTH_MAX];
  size_t recipient_count;
};

#endif // EVENTS_H
