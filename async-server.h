#ifndef RPC_SERVER_H
#define RPC_SERVER_H

#include <event.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <err.h>

/* server port number*/
#define SERVER_PORT 8080

/* Maximum connections allowed*/
#define BACKLOGS 10

/* Response buffer length */
#define RESPONSE_BUF_LENGTH 1024

/* Command arguments length */
#define MAX_ARGUMENTS 5

/* Command length */
#define CMD_LEN 4


#define PRINT_DIAG(x) do {printf("debug: %s:%d -> %s\n",__FILE__, __LINE__,x);} while(0)

/* client structure */
struct client {
  int fd;
  struct bufferevent *buf_ev;
};

#endif