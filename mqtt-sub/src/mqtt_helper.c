#include "mqtt_helper.h"
#include "events.h"
#include "helper.h"
#include "sqlite_helper.h"
#include "uci_helper.h"
#include <argp.h>
#include <mosquitto.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <uci.h>
#include <unistd.h>

bool stop_loop = false;
sqlite3 *db;

int subscribe_topic(struct mosquitto *mosq, struct topic topic) {
  int rc = mosquitto_subscribe(mosq, NULL, topic.name, topic.qos);
  if (rc != MOSQ_ERR_SUCCESS)
    syslog(LOG_ERR, "Failed subscription: %s, %s", topic.name,
           mosquitto_strerror(rc));
  return rc;
}

int subscribe_all_topics(struct mosquitto *mosq) {
  int subscribe_count = 0;
  struct topic topics[MQTT_TOPIC_CAP];
  int topic_count = get_all_topics(topics);
  if (topic_count < 0)
    return topic_count;

  for (int i = 0; i < topic_count; i++) {
    if (subscribe_topic(mosq, topics[i]) == MOSQ_ERR_SUCCESS) {
      subscribe_count++;
      syslog(LOG_INFO, "Subscribed to topic: %s", topics[i].name);
    }
  }

  return subscribe_count;
}

void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
  int rc;

  syslog(LOG_INFO, "on_connect: %s", mosquitto_connack_string(reason_code));
  if (reason_code != 0) {
    stop_loop = true;
    return;
  }

  rc = subscribe_all_topics(mosq);
  if (rc < 1) {
    syslog(LOG_ERR, "No topics subscribed: %s", mosquitto_strerror(rc));
    stop_loop = true;
  }
}

void on_message(struct mosquitto *mosq, void *obj,
                const struct mosquitto_message *msg) {
  time_t current_time = time(NULL);
  struct tm *timeinfo = localtime(&current_time);
  char date_buffer[128];

  strftime(date_buffer, sizeof(date_buffer), "%a, %d %b %Y %T %z", timeinfo);

  syslog(LOG_INFO, "At time: %s, received payload: %s, on topic: %s",
         date_buffer, (char *)msg->payload, msg->topic);
  int ret = check_and_send_events((char *)msg->payload, msg->topic);
  sqlite_insert(db, msg->payload, msg->topic);
}

int mqtt_init_login(struct mosquitto *mosq, struct arguments arguments) {
  int rc = MOSQ_ERR_SUCCESS;
  if (!arguments.username && !arguments.password)
    return rc;
  if ((rc = mosquitto_username_pw_set(mosq, arguments.username,
                                      arguments.password)) != MOSQ_ERR_SUCCESS)
    syslog(LOG_ERR, "No such user exists");

  return rc;
}

int mqtt_init_tls(struct mosquitto *mosq, struct arguments arguments) {
  int rc = MOSQ_ERR_SUCCESS;

  if (arguments.cafile != NULL) {
    if ((rc = mosquitto_tls_set(mosq, arguments.cafile, NULL,
                                arguments.certfile, arguments.keyfile, NULL)) !=
        MOSQ_ERR_SUCCESS) {
      syslog(LOG_ERR, "Failed to set TLS options: %s", mosquitto_strerror(rc));
      return rc;
    }
  }

  if (arguments.psk != NULL) {
    if ((rc = mosquitto_tls_psk_set(mosq, arguments.psk, arguments.psk_identity,
                                    NULL)) != MOSQ_ERR_SUCCESS) {
      syslog(LOG_ERR, "Failed to set TLS-PSK options: %s",
             mosquitto_strerror(rc));
      return rc;
    }
  }

  return rc;
}

int mqtt_init(struct mosquitto **mosq, struct arguments arguments) {
  int rc = MOSQ_ERR_SUCCESS;

  if ((rc = mosquitto_lib_init()) != MOSQ_ERR_SUCCESS) {
    syslog(LOG_ERR, "Failed to initialize mosquitto library: %s",
           mosquitto_strerror(rc));
    return rc;
  }

  *mosq = mosquitto_new(NULL, true, &arguments);

  if (*mosq == NULL) {
    syslog(LOG_ERR, "Failed to create mosquitto instance.");
    return MOSQ_ERR_UNKNOWN;
  }

  if ((rc = mqtt_init_login(*mosq, arguments)) != MOSQ_ERR_SUCCESS)
    return rc;

  if ((rc = mqtt_init_tls(*mosq, arguments)) != MOSQ_ERR_SUCCESS)
    return rc;

  mosquitto_connect_callback_set(*mosq, on_connect);
  mosquitto_message_callback_set(*mosq, on_message);

  syslog(LOG_INFO, "Initialized succesfully");
  return rc;
}

int mqtt_loop(struct mosquitto *mosq) {
  int rc = MOSQ_ERR_SUCCESS;
  while (!stop_loop) {
    rc = mosquitto_loop(mosq, -1, 1);
    switch (rc) {
    case MOSQ_ERR_CONN_LOST:
      syslog(LOG_ERR, "Connection to MQTT broker lost.");
      break;
    case MOSQ_ERR_NO_CONN:
      if (mqtt_attempt_reconnect(mosq) != MOSQ_ERR_SUCCESS)
        syslog(LOG_ERR, "Failed to reconnect to MQTT broker.");
      break;
    default:
      break;
    }
  }
  return rc;
}

int mqtt_attempt_reconnect(struct mosquitto *mosq) {
  int rc = MOSQ_ERR_SUCCESS;
  int reconnect_attempts = 0;

  while (reconnect_attempts < MQTT_RECONNECT_MAX_TRIES) {
    rc = mosquitto_reconnect(mosq);
    if (rc == MOSQ_ERR_SUCCESS) {
      syslog(LOG_INFO, "Reconnected to MQTT broker.");
      return MOSQ_ERR_SUCCESS;
    }

    reconnect_attempts++;
    sleep(1);
  }

  syslog(LOG_ERR, "Failed to reconnect to MQTT broker after maximum attempts.");
  return rc;
}

int mqtt_main(struct arguments arguments) {
  struct mosquitto *mosq = NULL;

  int rc = MOSQ_ERR_SUCCESS;
  int mqtt_init_rc = MOSQ_ERR_SUCCESS;
  int sqlite_init_rc = SQLITE_OK;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  openlog("mqtt_sub", LOG_PID, LOG_DAEMON);

  rc = uci_init();
  if (rc != UCI_OK) {
    closelog();
    return rc;
  }

  mqtt_init_rc = mqtt_init(&mosq, arguments);
  if (mqtt_init_rc != MOSQ_ERR_SUCCESS) {
    syslog(LOG_ERR, "Failed to initialize MQTT: %s",
           mosquitto_strerror(mqtt_init_rc));
    uci_deinit();
    mosquitto_lib_cleanup();
    closelog();
    return mqtt_init_rc;
  }

  sqlite_init_rc = sqlite_init(SQLITE_DATABASE_PATH, &db);
  if (sqlite_init_rc != SQLITE_OK) {
    syslog(LOG_ERR, "Failed to initialize SQLite: %s",
           sqlite3_errstr(sqlite_init_rc));
    uci_deinit();
    mosquitto_lib_cleanup();
    closelog();
    return sqlite_init_rc;
  }

  syslog(LOG_INFO, "Trying to establish connection to: %s:%d", arguments.host,
         arguments.port);
  rc = mosquitto_connect(mosq, arguments.host, arguments.port, 60);
  if (rc != MOSQ_ERR_SUCCESS) {
    syslog(LOG_ERR, "Failed to connect to MQTT broker: %s\n",
           mosquitto_strerror(rc));
    sqlite_deinit(db);
    uci_deinit();
    mosquitto_lib_cleanup();
    closelog();
    return rc;
  }

  if (arguments.daemon) {
    daemonize();
    syslog(LOG_INFO, "Daemon started");
  }

  mqtt_loop(mosq);

  sqlite_deinit(db);

  mosquitto_disconnect(mosq);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

  uci_deinit();

  closelog();

  return MOSQ_ERR_SUCCESS;
}
