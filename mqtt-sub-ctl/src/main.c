#include "helper.h"
#include "sock_helper.h"
#include <argp.h>
#include <stdlib.h>
#include <syslog.h>

static struct argp argp = {options, parse_opt, 0, doc};

struct data data = {
    .topics = {0},
    .topic_count = 0,
    .events = {0},
    .event_counts = {0},
};

int main(int argc, char *argv[]) {
  openlog("mqtt_sub_ctl", LOG_PID, LOG_DAEMON);

  if (argc == 1) {
    argp_help(&argp, stdout, ARGP_HELP_USAGE, argv[0]);
    closelog();
    return EXIT_SUCCESS;
  }

  argp_parse(&argp, argc, argv, 0, 0, NULL);

  closelog();

  return EXIT_SUCCESS;
}
