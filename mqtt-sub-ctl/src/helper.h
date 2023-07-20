#ifndef HELPER_H
#define HELPER_H

#include <argp.h>

#define BUFFER_SIZE 1024
#define PATH_MAX 4096

static char doc[] = "Controller for MQTT subscriber daemon program";

static struct argp_option options[] = {{"topic", 't', 0, 0},
                                       {"events", 'e', "<topic>", 0, 0},
                                       {"add_new_topic", 'T', "<topic>", 0, 0},
                                       {"add_new_event", 'E', "<event>", 0, 0},
                                       {0}};

error_t parse_opt(int key, char *arg, struct argp_state *state);
int file_exists(const char *filename);

#endif // HELPER_H
