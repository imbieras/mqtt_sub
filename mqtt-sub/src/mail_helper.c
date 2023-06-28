#include "mail_helper.h"
#include "helper.h"
#include "uci_helper.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

char payload_text[BUFFER_SIZE];

void form_payload_text(const char *recipient_email, const char *sender_email,
                       const char *subject, const char *message) {
  time_t current_time = time(NULL);

  struct tm *timeinfo = localtime(&current_time);
  char date_buffer[128];

  strftime(date_buffer, sizeof(date_buffer), "%a, %d %b %Y %T %z", timeinfo);

  snprintf(payload_text, BUFFER_SIZE,
           "Date: %s\r\n"
           "To: <%s>\r\n"
           "From: <%s>\r\n"
           "Subject: %s\r\n"
           "\r\n" // empty line to divide headers from body, see RFC5322
           "%s\r\n",
           date_buffer, recipient_email, sender_email, subject, message);
}

static size_t payload_source(char *ptr, size_t size, size_t nmemb,
                             void *userp) {
  struct upload_status *upload_ctx = (struct upload_status *)userp;
  const char *data;
  size_t room = size * nmemb;

  if ((size == 0) || (nmemb == 0) || ((size * nmemb) < 1)) {
    return 0;
  }

  data = &payload_text[upload_ctx->bytes_read];

  if (data) {
    size_t len = strlen(data);
    if (room < len)
      len = room;
    memcpy(ptr, data, len);
    upload_ctx->bytes_read += len;

    return len;
  }

  return 0;
}

void curl_set_sender_data(CURL *curl, struct sender_info info) {
  if (info.credentials) {
    curl_easy_setopt(curl, CURLOPT_USERNAME, info.username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, info.password);
    curl_easy_setopt(curl, CURLOPT_CAINFO, MAIL_CACERT_PATH);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
  }
  char url[BUFFER_SIZE];
  sprintf(url, "%s:%d", info.server, info.port);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_MAIL_FROM, info.sender_email);
}

int send_email(
    const char *sender_email,
    char recipient_emails[MAIL_MAX_RECIPIENTS][MAIL_FIELD_LENGTH_MAX],
    int recipient_count, const char *subject, const char *message) {
  struct upload_status upload_ctx;
  CURL *curl = curl_easy_init();
  CURLcode res;

  struct sender_info sender_info;
  get_sender_info(&sender_info, sender_email);

  syslog(LOG_INFO,
         "Sender info: server: %s:%d, credentials: %d, username: "
         "%s, sender email: %s",
         sender_info.server, sender_info.port, sender_info.credentials,
         sender_info.username, sender_info.sender_email);

  if (curl) {
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, sender_email);
    curl_set_sender_data(curl, sender_info);

    for (int i = 0; i < recipient_count; i++) {
      form_payload_text(recipient_emails[i], sender_email, subject, message);
      syslog(LOG_INFO, "Trying to send to recipient %s: message: %s",
             recipient_emails[i], message);
      upload_ctx.bytes_read = 0;

      curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipient_emails[i]);
      curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
      curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
      curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

      res = curl_easy_perform(curl);
      if (res != CURLE_OK) {
        syslog(LOG_ERR, "Failed to send email to recipient %s: %s",
               recipient_emails[i], curl_easy_strerror(res));
      }
    }

    curl_easy_cleanup(curl);
  } else {
    syslog(LOG_ERR, "Failed to initialize curl");
    res = CURLE_FAILED_INIT;
  }

  return res;
}
