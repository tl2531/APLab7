/*
 * Emily Schultz, ess2183
 * Lab7, part 2a/b
 *
 * http-server.c
 *
 * This program makes use of TCPEchoServer.c.
 */

#include <stdio.h>        /* for printf() and fprintf() */
#include <sys/socket.h>   /* for socket(), bind(), connect() */
#include <arpa/inet.h>    /* for sockaddr_in and inet_ntoa() */
#include <stdlib.h>       /* for atoi() and exit() */
#include <string.h>       /* for memset() */
#include <unistd.h>       /* for close() */
#include <signal.h>       /* for signal() */

#define MAXPENDING 5      /* maximum outstanding connection reqs */
#define SENDSIZE 4096     /* for sending client chunks of file */

static void die(const char *msg)
{
  perror(msg);
  exit(1);
}

void HandleTCPClient(int clntSocket);

int main(int argc, char **argv)
{

  int servSock;    /* Socket descriptor for server */
  int clntSock;    /* Socket descriptor for client */
  struct sockaddr_in servAddr; /* Local address */
  struct sockaddr_in clntAddr; /* Client address */
  unsigned short servPort;     /* Server port */
  unsigned int clntLen;      /* Length of client address data struct */

  if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    die("signal() failed");

  if(argc != 5)
    {
      fprintf(stderr, "Usage: %s <server_port> <web_root> <mdb-lookup-host> <mdb-lookup-port>\n", argv[0]);
      exit(1);
    }

  // parse args
  servPort = atoi(argv[1]); /* second arg: server port */
  char *webroot = argv[2];
  // for part 2b
  char *host = argv[3];
  unsigned short lookup_port = atoi(argv[4]);  /* fourth arg: lookup port */


  /* Create socket for incoming connections */
  if((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    die("socket() failed");

  /* Construct local address structure */
  memset(&servAddr, 0, sizeof(servAddr));
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(servPort);

  /* Bind to the local address */
  if(bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    die("bind() failed");

  /* Mark the socket so it will listen for incoming connections */
  if(listen(servSock, MAXPENDING) < 0)
    die("listen() failed");
  
  for (;;) /* Run forever */
    {
      /* Set the size of the in-out parameter */
      clntLen = sizeof(clntAddr);
      
      /* Wait for a client to connect */
      if ((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, 
			     &clntLen)) < 0)
	die("accept() failed");
      
      /* clntSock is connected to a client! */
      
      fprintf(stderr, "\nconnection started from: %s\n", 
	      inet_ntoa(clntAddr.sin_addr));
      
      HandleTCPClient(clntSock);
      
      fprintf(stderr, "connection terminated from: %s\n", 
	      inet_ntoa(clntAddr.sin_addr));
    }
  /* NOT REACHED */
}

void HandleTCPClient(int clntSocket)
{
  // Wrap the socket with a FILE* using fdopen()
  FILE *input = fdopen(clntSocket, "r"); 
  if (input == NULL)
    die("fdopen failed");

  char requestline[SENDSIZE];
  char *token_separators = "\t \r\n";
  int line = 0;
  while(fgets(requestline, sizeof(requestline), input) != NULL)
    {
      fprintf(stderr, "%s", requestline);
      if(line == 0)
	{
	  // parse the first line
	  char *method = strtok(requestline, token_separators);
	  char *requestURI = strtok(NULL, token_separators);
	  char *httpVersion = strtok(NULL, token_separators);
	  /* for debuggings */
	  fprintf(stderr, "\tMETHOD- %s, URI- %s, HTTP- %s.\n", method, requestURI, httpVersion);

	  if(strncmp("GET", method, strlen(method)) != 0)
	    {
	      // only support GET method
	      // 501 not implemented
	    }
	  if(strncmp("HTTP/1.0", httpVersion, 8) != 0 && strncmp("HTTP/1.1", httpVersion, 8) != 0)
	    { // must be either HTTP/1.0 or 1.1
	      // 501 not implemented
	    }
	  if(strncmp("/", requestURI, 1) != 0)
	    { // request URI must start with /
	      // 400 Bad Request 

	    }
	  line++;
	}


    }

  /*
  if(strncmp("HTTP/1.0 ", buf, 9) != 0 && strncmp("HTTP/1.1 ", buf, 9) != 0)
    {
      fprintf(stderr, "unknown protocol response: %s\n", buf);
      exit(1);
    }
  */


  /* CHANGE: we changed printf() into sprintf() & send()
  len = sprintf(out_buf, "%4d: {%s} said {%s}\n", recNo, rec->name, rec->msg);
  if ((res = send(clntSocket, out_buf, len, 0)) != len) 
  {
  perror("send() failed");
  break; 
  }*/
  
  // CHANGE: check 'input' rather than 'stdin'
  // see if fgets() produced error
  if (ferror(input)) 
      perror("fgets() failed");

  // Close the socket by closing the FILE* wrapper
  fclose(input);
}

