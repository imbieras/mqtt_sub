#include "helper.h"
#include "mqtt_helper.h"
#include <argp.h>
#include <mosquitto.h>
#include <stdlib.h>

static struct argp argp = {options, parse_opt, 0, doc};

int main(int argc, char *argv[]) {
  struct arguments arguments = {
      .daemon = false,
      .host = NULL,
      .port = 1883,
      .username = NULL,
      .password = NULL,
      .psk = NULL,
      .psk_identity = NULL,
      .cafile = NULL,
      .certfile = NULL,
      .keyfile = NULL,
  };

  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  if (mqtt_main(arguments) == MOSQ_ERR_SUCCESS) {
    return EXIT_SUCCESS;
  }
  return EXIT_FAILURE;
}
