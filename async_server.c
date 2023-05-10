/*
 * A server-side part of a client-server application that will execute commands from
 *  a client. You may use any transport protocol (UDP/TCP) to transfer data but explain your choice.
 *  The program should receive commands from a client, execute them and return a result to the
 *  client. A client might send several commands sequentially (after receiving a result to the
 *  previous command).
 */

#include "async_server.h"

/*
 * Function: resetInfo
 * --------------------
 *  clear all info context
 * info: info context
 * return: NA
 */
void freeInfo(struct info *info, struct bufferevent *bev)
{
  struct evbuffer *buffer = bufferevent_get_input(bev);
  info->cmd = 0;
  if (info->data)
  {
    free(info->data);
    info->data = NULL;
  }
  /* close the handle */
  if (info->fd != -1)
    close(info->fd);
  info->fd = -1;
  info->total_drained = 0;
  evbuffer_drain(buffer, -1);
}

/*
 * Function: handle_cat_command
 * --------------------
 * handl cat command
 * buffer :  event buffer
 * info: context, bev - buffer event
 * return: NA
 */
void handle_cat_command(struct evbuffer *buffer, struct info *info, struct bufferevent *bev)
{
  long length = evbuffer_get_length(buffer);
  char *buf = (char *)evbuffer_pullup(buffer, length);
  buf[length - 2] = '\0';
  int fd = open(buf, O_RDONLY);
  if (fd < 0)
  {
    bufferevent_write(bev, NO_SUCH_FILE, sizeof(NO_SUCH_FILE));
  }
  else
  {
    info->fd = fd;
    info->cmd = CMD_CAT;
    bufferevent_write(bev, "S ", sizeof("S "));
  }
  evbuffer_drain(buffer, length);
}

/*
 * Function: send_file
 * --------------------
 * send file data
 * buffer : info context
 *  bev - buffer event
 * return: NA
 */
int send_file(struct info *info, struct bufferevent *bev)
{
  // struct stat stats = {0};
  // struct evbuffer *outBuf = bufferevent_get_output(bev);

  // if (fstat(info->fd, &stats)<0) {
	// 		/* Make sure the length still matches, now that we
	// 		 * opened the file :/ */
	// 		perror("fstat");
	// 		return;
	// 	}
	// 	evbuffer_add_file(outBuf, info->fd, 0, stats.st_size);

  if (info->total_drained)
    lseek(info->fd, (size_t)info->total_drained, SEEK_SET);

  int bytes = evbuffer_read(bufferevent_get_output(bev), info->fd, MAX_LINE);
  if (bytes)
  {
    info->total_drained += bytes;
    return bytes;
  }
  return 0;
}

/*
 * Function: send_file
 * --------------------
 * send file data
 * buffer : info context
 *  bev - buffer event
 * return: NA
 */
int doSum(struct info *info, struct bufferevent *bev )
{
  char *pcToken;
  static long long sum = 0;

  pcToken = strtok(info->data + info->total_drained, " \n");
  if (pcToken != NULL)
  {
    sum += atol(pcToken);
    info->total_drained += strlen(pcToken) + 1; //space
    bufferevent_write(bev, " \b", 2);
    return 1;
  }
  evbuffer_add_printf(bufferevent_get_output(bev), "%lld\n", sum);
  sum=0;//clear sum
  return 0;
}

/*
 * Function:  async_read_callback
 * --------------------
 * read callback
 * bev : buffer event
 * ctx: context
 * return: NA
 */
static void
async_read_callback(struct bufferevent *bev, void *ctx)
{
  struct info *info = ctx;
  char *buf;
  /* This callback is invoked when there is data to read on bev. */
  struct evbuffer *buffer = bufferevent_get_input(bev);
  long length = evbuffer_get_length(buffer);
  int bytes;
  while (1)
  {
    // bytes = evbuffer_remove(buffer, cmd, sizeof(cmd));
    buf = (char *)evbuffer_pullup(buffer, CMD_LEN);
    if (buf ==  NULL) {
      printf("[READ] No more data: %ld\n",evbuffer_get_length(buffer));
      break; /* No more data. */
    }
      
    /*printf("[READ] Buffer :%s Total: %ld\n", buf, length);*/
    /* Handle ping */
    if ((!strncmp(buf, PING, CMD_LEN)) && (length <= 6))
    {
      info->cmd = CMD_PING;
      bufferevent_write(bev, SUCESS, sizeof(SUCESS));
      evbuffer_drain(buffer, length); //Remove all from the buffer
      continue;
    }
    else if (!strncmp(buf, CAT, CMD_LEN))
    {
      evbuffer_drain(buffer, CMD_LEN); // remove cmd from buffer
      /* Handle cat */
      handle_cat_command(buffer, info, bev);
      continue;
    }
    else if (!strncmp((const char *)buf, SUM, CMD_LEN))
    {
      /* Handle sum */
      info->cmd = CMD_SUM;
      info->data = (char *)calloc(0, length);
      bytes = evbuffer_remove(buffer, info->data, length);
      bufferevent_write(bev, SUCESS, sizeof(SUCESS));
      continue;
    }
    else
    {
      bufferevent_write(bev, INVALID_CMD, sizeof(INVALID_CMD));
      break;
    }
  }
  evbuffer_drain(buffer, -1);
  return;
}

/*
 * Function:  async_write_callback
 * --------------------
 * write callback
 * bev : buffer event
 * ctx: context
 * return: NA
 */
static void
async_write_callback(struct bufferevent *bev, void *ctx)
{
  struct info *info = ctx;
  printf("[WRITE] Command :%s\n", COMMAND[info->cmd]);
  switch (info->cmd)
  {
    case CMD_PING:
      bufferevent_write(bev, PONG, sizeof(PONG));
      break;

    case CMD_CAT:
      if (send_file(info, bev))
          return;
      bufferevent_write(bev, "\n", 1);
      break;
      
    case CMD_SUM:
      if (doSum(info, bev))
        return;
      break;

    default:
      printf("[WRITE] Invalid cmd: %d \n", info->cmd);
  }
  freeInfo(info, bev);
  return;
}

/*
 * Function:  async_event_callback
 * --------------------
 * event callback
 * bev : buffer event
 * ctx: context
 * return: NA
 */
static void
async_event_callback(struct bufferevent *bev, short what, void *ctx)
{

  if (what & BEV_EVENT_ERROR)
    perror("Error from bufferevent");

  int needfree = 0;

  if (what & BEV_EVENT_READING)
  {
    if (what & BEV_EVENT_EOF || what & BEV_EVENT_TIMEOUT)
      needfree = 1;
  }

  if (what & BEV_EVENT_ERROR)
  {
    needfree = 1;
  }
  /* free all resources */
  if (needfree)
  {
    int errcode = evutil_socket_geterror(bufferevent_getfd(bev));
    printf("[EVENT] errcode:%d\n", errcode);
    bufferevent_free(bev);
    free(ctx);
  }
}

/*
 * Function:  accept_conn_callback
 * --------------------
 *  accept connection callback
 * listener : buffer event, fd, address, socklen
 * ctx: context
 * return: NA
 */
static void
accept_conn_callback(struct evconnlistener *listener,
                     evutil_socket_t fd, struct sockaddr *address, int socklen,
                     void *ctx)
{
  struct info *ctxInfo;
  ctxInfo = malloc(sizeof(struct info));
  ctxInfo->cmd = 0; // invalid cmd
  ctxInfo->data = NULL;
  ctxInfo->total_drained = 0;
  ctxInfo->fd = -1;

  /* We got a new connection! Set up a bufferevent for it. */
  struct event_base *base = evconnlistener_get_base(listener);
  struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

  bufferevent_setcb(bev, async_read_callback, async_write_callback, async_event_callback, ctxInfo);

  bufferevent_enable(bev, EV_READ | EV_WRITE);
}

/*
 * Function:  accept_error_callback
 * --------------------
 *  accept err callback
 * listener : buffer event, fd, address, socklen
 * ctx: context
 * return: NA
 */
static void
accept_error_callback(struct evconnlistener *listener, void *ctx)
{
  struct event_base *base = evconnlistener_get_base(listener);
  int err = EVUTIL_SOCKET_ERROR();
  fprintf(stderr, "Got an error %d (%s) on the listener. "
                  "Shutting down.\n",
          err, evutil_socket_error_to_string(err));

  if (ctx)
    free(ctx);
  event_base_loopexit(base, NULL);
}

/*
 * Function:  signal_callack
 * --------------------
 *  singanl callback
 * sig : sig event, events - event number
 * ctx: context
 * return: NA
 */
static void
signal_callack(evutil_socket_t sig, short events, void *ctx)
{
  struct event_base *base = ctx;
  struct timeval delay = {1, 0};

  printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

  event_base_loopexit(base, &delay);
}

int main(int argc, char **argv)
{
  struct event_base *base;
  struct evconnlistener *listener;
  struct sockaddr_in sin;
  struct event *signal_event;

  int port = 8080;

  base = event_base_new();
  if (!base)
  {
    puts("Couldn't open event base");
    return 1;
  }

  memset(&sin, 0, sizeof(sin));
  /*This is an INET address */
  sin.sin_family = AF_INET;
  /*Listen on 0.0.0.0 */
  sin.sin_addr.s_addr = htonl(0);
  /*Listen on the given port. */
  sin.sin_port = htons(port);

  listener = evconnlistener_new_bind(base, accept_conn_callback, NULL,
                                     LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1,
                                     (struct sockaddr *)&sin, sizeof(sin));
  if (!listener)
  {
    perror("Couldn't create listener");
    return 1;
  }

  signal_event = evsignal_new(base, SIGINT, signal_callack, (void *)base);

  if (!signal_event || event_add(signal_event, NULL) < 0)
  {
    fprintf(stderr, "Could not create/add a signal event!\n");
    return 1;
  }

  evconnlistener_set_error_cb(listener, accept_error_callback);
  event_enable_debug_logging(EVENT_DBG_NONE);
  event_base_dispatch(base);

  /* free respurce */
  evconnlistener_free(listener);
  event_free(signal_event);
  event_base_free(base);

  return 0;
}