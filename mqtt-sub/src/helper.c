#include "helper.h"
#include <argp.h>
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

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    syslog(LOG_WARNING, "Signal received. Stopping.");
    stop_loop = true;
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
