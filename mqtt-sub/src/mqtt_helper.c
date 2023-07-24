#include "mqtt_helper.h"
#include "events.h"
#include "helper.h"
#include "sock_helper.h"
#include "sqlite_helper.h"
#include "uci_helper.h"
#include <argp.h>
#include <mosquitto.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <uci.h>
#include <unistd.h>

struct mosquitto *mosq = NULL;
bool stop_loop = false;
struct server_data server;
pthread_t connection_thread;
sqlite3 *db;

struct data data = {
    .topics = {0},
    .topic_count = 0,
    .events = {0},
    .event_counts = {0},
};

static int subscribe_topic(struct topic topic) {
  int rc = mosquitto_subscribe(mosq, NULL, topic.name, topic.qos);
  if (rc != MOSQ_ERR_SUCCESS)
    syslog(LOG_ERR, "Failed subscription: %s, %s", topic.name,
           mosquitto_strerror(rc));
  return rc;
}

int cache_topic_events() {
  for (size_t i = 0; i < data.topic_count; i++) {
    size_t tmp_event_count =
        get_all_topic_events(data.events[i], data.topics[i].name);
    if (tmp_event_count >= 0) {
      data.event_counts[i] = tmp_event_count;
    }
  }

  return EXIT_SUCCESS;
}

size_t subscribe_all_topics() {
  size_t subscribe_count = 0;

  data.topic_count = get_all_topics(data.topics);
  if (data.topic_count < 0)
    return data.topic_count;

  cache_topic_events();

  for (size_t i = 0; i < data.topic_count; i++) {
    if (subscribe_topic(data.topics[i]) == MOSQ_ERR_SUCCESS) {
      subscribe_count++;
      syslog(LOG_INFO, "Subscribed to topic: %s", data.topics[i].name);
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

  rc = subscribe_all_topics();
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
  check_and_send_events((char *)msg->payload, msg->topic);
  sqlite_insert(db, msg->payload, msg->topic);
}

static int mqtt_init_login(struct arguments arguments) {
  int rc = MOSQ_ERR_SUCCESS;
  if (!arguments.username && !arguments.password)
    return rc;
  if ((rc = mosquitto_username_pw_set(mosq, arguments.username,
                                      arguments.password)) != MOSQ_ERR_SUCCESS)
    syslog(LOG_ERR, "No such user exists");

  return rc;
}

static int mqtt_init_tls(struct arguments arguments) {
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

static int mqtt_attempt_reconnect() {
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

static int mqtt_init(struct arguments arguments) {
  int rc = MOSQ_ERR_SUCCESS;

  if ((rc = mosquitto_lib_init()) != MOSQ_ERR_SUCCESS) {
    syslog(LOG_ERR, "Failed to initialize mosquitto library: %s",
           mosquitto_strerror(rc));
    return rc;
  }

  mosq = mosquitto_new(NULL, true, &arguments);

  if (mosq == NULL) {
    syslog(LOG_ERR, "Failed to create mosquitto instance.");
    return MOSQ_ERR_UNKNOWN;
  }

  if ((rc = mqtt_init_login(arguments)) != MOSQ_ERR_SUCCESS)
    return rc;

  if ((rc = mqtt_init_tls(arguments)) != MOSQ_ERR_SUCCESS)
    return rc;

  mosquitto_connect_callback_set(mosq, on_connect);
  mosquitto_message_callback_set(mosq, on_message);

  syslog(LOG_INFO, "Initialized succesfully");
  return rc;
}

static int mqtt_loop() {
  int rc = MOSQ_ERR_SUCCESS;
  while (!stop_loop) {
    rc = mosquitto_loop(mosq, -1, 1);
    switch (rc) {
    case MOSQ_ERR_CONN_LOST:
      syslog(LOG_ERR, "Connection to MQTT broker lost.");
      break;
    case MOSQ_ERR_NO_CONN:
      if (mqtt_attempt_reconnect() != MOSQ_ERR_SUCCESS)
        syslog(LOG_ERR, "Failed to reconnect to MQTT broker.");
      break;
    default:
      break;
    }
  }
  return rc;
}

int mqtt_main(struct arguments arguments) {
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

  mqtt_init_rc = mqtt_init(arguments);
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

  init_server_socket(&server);
  server.subscriber_data = &data;

  if (pthread_create(&connection_thread, NULL, handle_connection, &server) !=
      0) {
    syslog(LOG_ERR, "pthread_create %m");
    deinit_server_socket(&server);
    exit(EXIT_FAILURE);
  }

  mqtt_loop();

  sqlite_deinit(db);
  mosquitto_disconnect(mosq);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();

  uci_deinit();

  closelog();

  return MOSQ_ERR_SUCCESS;
}
