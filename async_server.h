#ifndef RPC_SERVER_H
#define RPC_SERVER_H
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include<fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define EVENT_DBG_NONE 0
#define EVENT_DBG_ALL 0xffffffffu

/* Command length */
#define CMD_LEN 4

/* Command arguments length */
#define MAX_ARGUMENTS 5

#define PING "ping"

#define PONG "pong\n"

#define SUCESS "S "

#define CAT "cat "

#define SUM "sum "

#define NO_SUCH_FILE "E No such file\n"

#define INVALID_CMD "E Invalid command....\n"

#define CMD_PING 1

#define CMD_CAT 2

#define CMD_SUM 3

#define MAX_LINE    256

/* structure to hold info between async call */
struct info {
  int cmd;
  char *data;
  size_t total_drained;
  int fd;
};

char *COMMAND[] = {"Invalid", "ping", "cat", "sum"};
#endif