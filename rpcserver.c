/*
 *  A server-side part of a client-server application that will execute commands from
 *  a client. You may use any transport protocol (UDP/TCP) to transfer data but explain your choice.
 *  The program should receive commands from a client, execute them and return a result to the
 *  client. A client might send several commands sequentially (after receiving a result to the
 *  previous command).
 */

#include "rpcserver.h"

/*
 * Function:  setnonblock 
 * --------------------
 * Sets the network socket to non-blocking I/O.
 * fd: socket fd   
 * 
 * return: 0 Success -1 Error
 */

int setnonblock(int ifd)
{
  int iflags;

  iflags = fcntl(ifd, F_GETFL);
  iflags |= O_NONBLOCK;
  fcntl(ifd, F_SETFL, iflags);
  return 0;
}

/*
 * Function:  parse_command 
 * --------------------
 * handle sum command
 * pcReq : command argument string
 * pcArguments: list of arguments
 * return: NA
 */
int parse_command(char *pcReq, char *pcArguments[MAX_ARGUMENTS]) {
    char *pcToken;
    int iIndex = 0;

    /* validate i/p */
    if (NULL == pcReq || NULL == pcArguments){
      return 0;
    }
    pcToken = strtok(pcReq, " \n");
    while (pcToken != NULL && iIndex < MAX_ARGUMENTS) {
        pcArguments[iIndex++] = pcToken;
        pcToken = strtok(NULL, " \n");
    }
    pcArguments[iIndex] = NULL;
    return iIndex;
}

/*
 * Function:  handle_sum_command 
 * --------------------
 * handle ping command
 * pcReq - input buffer 
 * response - response output buffer
 * return: NA
 */
void handle_ping_command(char *pcReq, char *pcResponse)
{
  char *pcArguments[MAX_ARGUMENTS];
  
  /* validate i/p */
  if (NULL == pcReq || NULL == pcResponse){
      return;
  }

  if (strlen(pcReq) < CMD_LEN + 1){
    int iArgC = parse_command(pcReq, pcArguments);
    if (!(strcmp(pcArguments[0], "ping")) && (1 == iArgC)){ 
        sprintf(pcResponse, "S pong\n");
        return;
    }    
  }
  sprintf(pcResponse, "E Invalid command....\n");
  return;
}

/*
 * Function:  handle_cat_command 
 * --------------------
 * handle sum command
 * argc - number of argument, filename - name of the file
 * response - response output buffer
 * return: NA
 */
void handle_cat_command(int iArgc, char *pcFilename, char *pcResponse, struct evbuffer *pstEvreturn)
{
   /* validate i/p */
  if (NULL == pcFilename || NULL == pcResponse || NULL == pstEvreturn){
      return;
  }

  if (iArgc < 2) {
      sprintf(pcResponse, "E file name is required.\n");
      return;
  } else {
      FILE *fp = fopen(pcFilename, "r");
      if (NULL == fp) {
          sprintf(pcResponse, "E No such file.\n");
      } else {
          /* Read file contents */
          char ch;
          int iIndex=0;
          pcResponse[iIndex++] = 'S';
          pcResponse[iIndex++] = ' ';
          while ((ch = fgetc(fp)) != EOF) {
              if (iIndex >= RESPONSE_BUF_LENGTH - 3) { /* handle large size files*/
                pcResponse[iIndex] = '\0';
                evbuffer_add_printf(pstEvreturn,"%s",pcResponse);
                iIndex = 0;
              }
              pcResponse[iIndex++] = ch;
          }
          pcResponse[iIndex] = '\0'; 
          sprintf(pcResponse, "%s\n", pcResponse);
          fclose(fp);
      }
   }
   return;
} 

/*
 * Function:  handle_sum_command 
 * --------------------
 * handle sum command
 * pcReq : command argument string
 * response - response output buffer
 * return: NA
 */
void handle_sum_command(char *pcReq, char *pcResponse)
{
  char *pcToken;
  long sum=0;

  /* validate i/p */
  if (NULL == pcReq){
      return;
  }

  pcToken = strtok(pcReq, " \n");
  while (pcToken != NULL) {
      sum += atoi(pcToken);
      pcToken = strtok(NULL, " \n");
  }
  sprintf(pcResponse, "S %ld\n", sum);
  return;
}

/*
 * Function:  buf_read_callback 
 * --------------------
 * Read buffer callback
 * pstIncomingBufEvent - incoming buffer event 
 *
 * return: NA
 */
void buf_read_callback(struct bufferevent *pstIncomingBufEvent, void *ctx) 
{
    struct evbuffer *pstEvreturn;
    char cResponse[RESPONSE_BUF_LENGTH]= {0};
    char *pcArguments[MAX_ARGUMENTS];
    int  iArgC;
    char *pcReq;

    /* validate i/p */
    if (NULL == pstIncomingBufEvent){
      return;
    }

    /* Read command buffer from client */
    while(1){
      pcReq = evbuffer_readline(pstIncomingBufEvent->input);
      if (pcReq == NULL)
         return;

      pstEvreturn = evbuffer_new();
          
      /* Execute command */
      if (strncmp(pcReq, "ping", CMD_LEN) == 0) {
          /* Ping command */
          handle_ping_command(pcReq, cResponse);
      } else if (strncmp(pcReq, "cat ", CMD_LEN) == 0) {
          /* Cat command */
          iArgC = parse_command(pcReq, pcArguments);
          handle_cat_command(iArgC, pcArguments[1], cResponse, pstEvreturn);
      } else if (strncmp(pcReq, "sum ", CMD_LEN) == 0) {
          /* Sum command */
          handle_sum_command(pcReq, cResponse);  
      } else {
          /* Invalid command */
          sprintf(cResponse, "E Invalid command....\n");
      }

      /* Send response to client */
      evbuffer_add_printf(pstEvreturn,"%s",cResponse);
      bufferevent_write_buffer(pstIncomingBufEvent, pstEvreturn);

      /* free the resource */
      evbuffer_free(pstEvreturn);
      free(pcReq);
    }
}

/*
 * Function:  buf_write_callback 
 * --------------------
 * Write buffer callback
 * 
 *
 * return: NA
 */
void buf_write_callback(struct bufferevent *pstBufEvent, void *pvArg)
{
  PRINT_DIAG("Write buffer call.");
}

/*
 * Function:  buf_error_callback 
 * --------------------
 * Error buffer callback
 * 
 *
 * return: NA
 */
void buf_error_callback(struct bufferevent *pstBufEvent, short shWhat, void *arg)
{
  struct client *stClient = (struct client *)arg;
  PRINT_DIAG("Err buffer call.");
  /* free the resource */
  bufferevent_free(stClient->buf_ev);
  close(stClient->fd);
  free(stClient);
}

/*
 * Function:  accept_callback 
 * --------------------
 * Accept buffer callback
 *  ifd - socket descriptor for the connection
 *
 * return: NA
 */
void accept_callback(int ifd, short shEv, void *arg)
{
  PRINT_DIAG("Accepted the connection.");
  int iclientFd;
  struct sockaddr_in stClient_addr;
  socklen_t clientLen = sizeof(stClient_addr);
  struct client *pstClient;

  iclientFd = accept(ifd,
                     (struct sockaddr *)&stClient_addr,
                     &clientLen);
  if (iclientFd < 0)
  {
    PRINT_DIAG("accept() failed.");
    return;
  }

  setnonblock(iclientFd);

  pstClient = calloc(1, sizeof(*pstClient));
  if (NULL == pstClient){
    PRINT_DIAG("accept mem alloc failed.");
    return;
  }

  pstClient->fd = iclientFd;

  pstClient->buf_ev = bufferevent_new(iclientFd,
                                   buf_read_callback,
                                   buf_write_callback,
                                   buf_error_callback,
                                   pstClient);

  bufferevent_enable(pstClient->buf_ev, EV_READ);
}

/*
 * Function:  main 
 * --------------------
 * Main routine for the server.
 * 
 *
 * return: 0 Success or Non-zero Failure
 */
int main()
{
  int iSockListen;
  struct sockaddr_in stAddressListen;
  struct event stAcceptEvent;
  int iReuse = 1;

  event_init();

  iSockListen = socket(AF_INET, SOCK_STREAM, 0);

  if (iSockListen < 0)
    {
      fprintf(stderr,"Failed to create listen socket");
      return 1;
    }

  memset(&stAddressListen, 0, sizeof(stAddressListen));

  stAddressListen.sin_family = AF_INET;
  stAddressListen.sin_addr.s_addr = INADDR_ANY;
  stAddressListen.sin_port = htons(SERVER_PORT);

  if (bind(iSockListen,
           (struct sockaddr *)&stAddressListen,
           sizeof(stAddressListen)) < 0)
  {
    fprintf(stderr,"Failed to bind");
    return 1;
  }

  PRINT_DIAG("Server starting....");

  if (listen(iSockListen, BACKLOGS) < 0)
  {
    fprintf(stderr,"Failed to listen to socket");
    return 1;
  }

  setsockopt(iSockListen, SOL_SOCKET, SO_REUSEADDR, &iReuse, sizeof(iReuse));
  
  /* Sets the socket to non-blocking */
  setnonblock(iSockListen);

  /* Constuct the new event structure */
  event_set(&stAcceptEvent, iSockListen, EV_READ|EV_PERSIST,  accept_callback, NULL);

  /* Adds the event to the queue */
  event_add(&stAcceptEvent, NULL);
  
  /* Start the event queue and waiting for request */
  event_dispatch();

  /* Close the listen socket */
  close(iSockListen);

  return 0;
}
