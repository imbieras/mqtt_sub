#include "helper.h"
#include "sock_helper.h"
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

const char *argp_program_version = "mqtt_sub_ctl 0.1";
error_t argp_err_exit_status = 1;

char request[BUFFER_SIZE];

error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = (struct arguments *)state->input;
  switch (key) {
  case 't':
    send_request_and_receive_response("return_all_topics");
    break;
  case 'e':
    snprintf(request, sizeof(request), "%s:%s", "return_events_for_topic", arg);
    send_request_and_receive_response(request);
    break;
  case 'T':
    snprintf(request, sizeof(request), "%s:%s", "add_new_topic", arg);
    send_request_and_receive_response(request);
    break;
  case 'E':
    snprintf(request, sizeof(request), "%s:%s", "add_new_event", arg);
    send_request_and_receive_response(request);
    break;
  case ARGP_KEY_END:
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}
