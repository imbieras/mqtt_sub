#include "sock_helper.h"
#include "helper.h"
#include "uci_helper.h"
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

const char sockname[] = "/tmp/sub_socket";
extern bool stop_loop;

void send_response(int client_socket, const char *response) {
  write(client_socket, response, strlen(response));
}

void return_all_topics(int client_socket, struct data *subscriber_data) {
  char response[4 * BUFFER_SIZE] = {0};
  int response_len = 0;

  for (int i = 0; i < subscriber_data->topic_count; ++i) {
    response_len += snprintf(
        response + response_len, sizeof(response) - response_len,
        "Topic %d: %s (QoS %d)\n", i + 1, subscriber_data->topics[i].name,
        subscriber_data->topics[i].qos);
  }

  send_response(client_socket, response);
}

void return_events_for_topic(int client_socket, struct data *subscriber_data,
                             const char *topic_name) {
  char response[4 * BUFFER_SIZE] = {0};
  int response_len = 0;
  int topic_index = -1;

  for (int i = 0; i < subscriber_data->topic_count; ++i) {
    if (strcmp(subscriber_data->topics[i].name, topic_name) == 0) {
      topic_index = i;
      break;
    }
  }

  if (topic_index == -1) {
    send_response(client_socket, "Topic not found.\n");
    return;
  }

  for (int i = 0; i < subscriber_data->event_counts[topic_index]; ++i) {
    struct event *current_event = &subscriber_data->events[topic_index][i];

    response_len +=
        snprintf(response + response_len, sizeof(response) - response_len,
                 "#%d Event:\r\nTopic - %s\r\nKey - %s\r\nComparison value - "
                 "%s\r\nComparison type - %s\r\n\r\n",
                 i + 1, current_event->topic.name, current_event->key,
                 current_event->value, current_event->comparison_type);
  }

  send_response(client_socket, response);
}

void handle_controller_request(int client_socket,
                               struct data *subscriber_data) {
  char buffer[4 * BUFFER_SIZE] = {0};
  ssize_t recv_bytes = recv(client_socket, buffer, sizeof(buffer), 0);

  if (recv_bytes <= 0) {
    if (recv_bytes < 0) {
      syslog(LOG_ERR, "recv: %m");
    }
    return;
  }

  if (strcmp(buffer, "return_all_topics") == 0) {
    return_all_topics(client_socket, subscriber_data);
  } else if (strncmp(buffer, "return_events_for_topic:", 24) == 0) {
    return_events_for_topic(client_socket, subscriber_data, buffer + 24);
  } else if (strncmp(buffer, "add_new_topic:", 14) == 0) {
    insert_topic_to_mqtt_sub(client_socket, buffer + 14);
  } else if (strncmp(buffer, "add_new_event:", 14) == 0) {
    insert_event_to_mqtt_sub(client_socket, buffer + 14);
  } else {
    send_response(client_socket, "Invalid request.");
  }
}

void *handle_connection(void *arg) {
  struct server_data *server = (struct server_data *)arg;

  while (!stop_loop) {
    pthread_testcancel();
    syslog(LOG_INFO, "Waiting for a connection...");

    int client_socket = accept(server->server_socket, NULL, NULL);
    if (client_socket == -1) {
      syslog(LOG_INFO, "accept: %m");
      continue;
    }

    syslog(LOG_INFO, "Connection accepted.");

    handle_controller_request(client_socket, server->subscriber_data);

    close(client_socket);
  }

  return NULL;
}

void init_server_socket(struct server_data *server) {
  struct sockaddr_un addr;

  server->server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (server->server_socket == -1) {
    syslog(LOG_INFO, "socket: %m");
    exit(EXIT_FAILURE);
  }

  int reuse_addr = 1;
  if (setsockopt(server->server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_addr,
                 sizeof(reuse_addr)) == -1) {
    syslog(LOG_INFO, "setsockopt: %m");
    close(server->server_socket);
    exit(EXIT_FAILURE);
  }

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sockname, sizeof(addr.sun_path) - 1);

  if (bind(server->server_socket, (struct sockaddr *)&addr,

           sizeof(struct sockaddr_un)) == -1) {
    syslog(LOG_INFO, "bind: %m");
    close(server->server_socket);
    exit(EXIT_FAILURE);
  }

  if (listen(server->server_socket, 1) == -1) {
    syslog(LOG_INFO, "listen: %m");
    close(server->server_socket);
    exit(EXIT_FAILURE);
  }
}

void deinit_server_socket(struct server_data *server) {
  close(server->server_socket);
  unlink(sockname);
}
