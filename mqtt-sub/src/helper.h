#ifndef HELPER_H
#define HELPER_H

#include <argp.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define PATH_MAX 4096

static char doc[] = "A simple MQTT subscriber daemon program";

static struct argp_option options[] = {
    {"daemon", 'a', 0, 0},
    {"host", 'h', "<host>", 0, 0},
    {"port", 'p', "<port>", 0, 0},
    {"username", 'u', "<username>", 0, 0},
    {"password", 'P', "<password>", 0, 0},
    {"psk", 's', "<key>", 0, 0},
    {"psk_identity", 'i', "<username>", 0, 0},
    {"cafile", 'C', "<path>", 0, 0},
    {"certfile", 'c', "<path>", 0, 0},
    {"keyfile", 'k', "<path>", 0, 0},
    {0}};

struct arguments {
  bool daemon;
  char *host;
  int port;
  char *username;
  char *password;
  char *psk;
  char *psk_identity;
  char *cafile;
  char *certfile;
  char *keyfile;
};

void signal_handler(int signal);
error_t parse_opt(int key, char *arg, struct argp_state *state);
void daemonize();
int create_file_if_not_exists(const char *filename);

#endif // HELPER_H
