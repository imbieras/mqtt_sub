#include "helper.h"
#include "pthread.h"
#include "sock_helper.h"
#include <argp.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

const char *argp_program_version = "mqtt_sub 0.1";
error_t argp_err_exit_status = 1;
extern bool stop_loop;
extern pthread_t connection_thread;
extern struct server_data server;

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    syslog(LOG_WARNING, "Signal received. Stopping.");
    stop_loop = true;

    pthread_cancel(connection_thread);
    pthread_join(connection_thread, NULL);

    deinit_server_socket(&server);
  }
}

error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = (struct arguments *)state->input;
  switch (key) {
  case 'a':
    arguments->daemon = true;
    break;
  case 'h':
    arguments->host = arg;
    break;
  case 'p':
    arguments->port = atoi(arg);
    break;
  case 'u':
    arguments->username = arg;
    break;
  case 'P':
    arguments->password = arg;
    break;
  case 's':
    arguments->psk = arg;
    break;
  case 'i':
    arguments->psk_identity = arg;
    break;
  case 'C':
    arguments->cafile = arg;
    break;
  case 'c':
    arguments->certfile = arg;
    break;
  case 'k':
    arguments->keyfile = arg;
    break;
  case ARGP_KEY_END:
    if (arguments->host == NULL) {
      syslog(LOG_ERR, "Host must be provided");
      argp_failure(state, 1, 0, "Host must be provided");
    }
    if ((arguments->cafile != NULL) && (arguments->psk != NULL))
      argp_failure(state, 1, 0, "Choose either certificate or preshared key");
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

void daemonize() {
  pid_t pid, sid;

  pid = fork();
  if (pid < 0) {
    syslog(LOG_ERR, "Failed to fork: %d", pid);
    exit(EXIT_FAILURE);
  }
  if (pid > 0) {
    exit(EXIT_SUCCESS);
  }

  sid = setsid();
  if (sid < 0) {
    syslog(LOG_ERR, "Failed to setsid: %d", sid);
    exit(EXIT_FAILURE);
  }

  umask(0);

  if (chdir("/") < 0) {
    syslog(LOG_ERR, "Failed to chdir to /");
    exit(EXIT_FAILURE);
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
}

static int file_exists(const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file) {
    fclose(file);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

static int create_empty_file(const char *filename) {
  FILE *file = fopen(filename, "w");
  if (file) {
    fclose(file);
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}

int create_file_if_not_exists(const char *file_path) {
  if (file_exists(file_path) != EXIT_SUCCESS) {
    char dir[PATH_MAX];
    strncpy(dir, file_path, sizeof(dir));
    char *slash = strrchr(dir, '/');
    if (slash) {
      *slash = '\0';
      struct stat st;
      if (stat(dir, &st) == -1) {
        if (mkdir(dir, 0700) == -1) {
          syslog(LOG_ERR, "Failed to create directory: %s", slash);
          return EXIT_FAILURE;
        }
      }
    }
  }

  if (create_empty_file(file_path) != EXIT_SUCCESS) {
    syslog(LOG_ERR, "Failed to create file: %s", file_path);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
