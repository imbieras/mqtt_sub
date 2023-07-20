#include "sock_helper.h"
#include "helper.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <syslog.h>
#include <unistd.h>

const char sockname[] = "/tmp/sub_socket";

void send_request_and_receive_response(const char *request) {
  struct sockaddr_un addr;
  int client_socket;
  char buffer[BUFFER_SIZE];

  client_socket = socket(AF_UNIX, SOCK_STREAM, 0);
  if (client_socket == -1) {
    syslog(LOG_ERR, "socket %m");
    exit(EXIT_FAILURE);
  }

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, sockname, sizeof(addr.sun_path) - 1);

  if (connect(client_socket, (struct sockaddr *)&addr,
              sizeof(struct sockaddr_un)) == -1) {
    syslog(LOG_ERR, "connect %m");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  if (send(client_socket, request, strlen(request), 0) == -1) {
    syslog(LOG_ERR, "send %m");
    close(client_socket);
    exit(EXIT_FAILURE);
  }

  ssize_t recv_bytes;
  while ((recv_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0)) >
         0) {
    buffer[recv_bytes] = '\0';
    printf("%s", buffer);
  }

  if (recv_bytes == -1) {
    syslog(LOG_ERR, "recv %m");
  }

  close(client_socket);
}
