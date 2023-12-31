#ifndef MAIL_HELPER_H
#define MAIL_HELPER_H

#include <curl/curl.h>
#include <stdlib.h>

#define MAIL_FIELD_LENGTH_MAX 256
#define MAIL_MAX_RECIPIENTS 16
#define MAIL_CACERT_PATH "/etc/certificates/ca.cert.pem"

struct sender_info {
  char server[MAIL_FIELD_LENGTH_MAX];
  int port;
  int credentials;
  char username[MAIL_FIELD_LENGTH_MAX];
  char password[MAIL_FIELD_LENGTH_MAX];
  char sender_email[MAIL_FIELD_LENGTH_MAX];
};

struct upload_status {
  size_t bytes_read;
};

int send_email(
    const char *sender_email,
    char recipient_emails[MAIL_MAX_RECIPIENTS][MAIL_FIELD_LENGTH_MAX],
    int recipient_count, const char *subject, const char *message);

#endif // MAIL_HELPER_H
