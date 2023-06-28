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

void form_payload_text(const char *recipient, const char *sender,
                       const char *subject, const char *message);
static size_t payload_source(char *ptr, size_t size, size_t nmemb, void *userp);
void curl_set_sender_data(CURL *curl, struct sender_info info);
int send_email(
    const char *sender_email,
    char recipient_emails[MAIL_MAX_RECIPIENTS][MAIL_FIELD_LENGTH_MAX],
    int recipient_count, const char *subject, const char *message);

#endif // MAIL_HELPER_H
