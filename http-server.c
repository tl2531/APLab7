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

void HandleTCPClient(int clntSocket, char *web_root);

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
      fprintf(stderr,"Usage: %s <server_port> <web_root> "
	      "<mdb-lookup-host> <mdb-lookup-port>\n", argv[0]);
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
      
      HandleTCPClient(clntSock, webroot);
      
      fprintf(stderr, "connection terminated from: %s\n", 
	      inet_ntoa(clntAddr.sin_addr));
    }
  /* NOT REACHED */
}

void HandleTCPClient(int clntSocket, char* web_root)
{
  // Wrap the socket with a FILE* using fdopen()
  FILE *input = fdopen(clntSocket, "r"); 
  if (input == NULL)
    die("fdopen failed");

  char requestline[SENDSIZE];
  char out_buffer[SENDSIZE];
  char *token_separators = "\t \r\n";
  if(recv(clntSocket,requestline, sizeof(requestline), 0) > 0)
    {
      fprintf(stderr, "%s", requestline);
      // parse the first line
      char *method = strtok(requestline, token_separators);
      char *requestURI = strtok(NULL, token_separators);
      char *httpVersion = strtok(NULL, token_separators);
      /* for debugging
	 fprintf(stderr, "METHOD- %s, URI- %s, HTTP- %s.\n",method, requestURI, httpVersion);*/

      if((strncmp("GET", method, strlen(method)) != 0) || 
	 (strncmp("HTTP/1.0", httpVersion, 8) != 0 && 
	  strncmp("HTTP/1.1", httpVersion, 8) != 0))
	{ // only support GET method, must be either HTTP/1.0 or 1.1
	  // otherwise send 501 not implemented
	  int length = sprintf(out_buffer, "HTTP/1.0 501 Not Implemented"
		"\n\n<html><body><h1>501 Not Implemented</h1></body></html>");
	  if(send(clntSocket, out_buffer, length, 0) != length)
	    {
	      perror("send() failed");
	      fclose(input);
	    }
	}
      else if(strncmp("/", requestURI, 1) != 0)
	{ // request URI must start with /
	  // otherwise send 400 bad request
	  int length = sprintf(out_buffer, "400 Bad Request");
	  if(send(clntSocket, out_buffer, length, 0) != length)
	    {
	      perror("send() failed");
	      fclose(input);
	    }
	}
      // successful request, send what they ask for
      else
	{
	  int linesread = 0;
	  char webfile[strlen(web_root) + strlen(requestURI) + 11];
	  strncpy(webfile, web_root, sizeof(webfile));
	  strcat(webfile, requestURI);
	  if(requestURI[strlen(requestURI)-1] == '/')
	    { //if it ends in /, append index.html
	      char *end = "index.html";
	      strcat(webfile, end);
	    }
	  fprintf(stderr, "%s\n", webfile);
	  FILE* read_file = fopen(webfile, "rb");

	  while(fread(out_buffer, 1, SENDSIZE, read_file) != 0)
	    {
	      //int len = sprintf(out_buffer, *whatever we just read*);
	      if(send(clntSocket, out_buffer, strlen(out_buffer), 0) 
		 !=  strlen(out_buffer))
		{
		  perror("send() failed");
		  break;
		}
	      fprintf(stderr, "%s\n", out_buffer);
	    }
	  // if haven't sent anything
	  if(linesread == 0)
	    {
	      //file not found  - 404?
	      
	    }
	}
    }

  // CHANGE: check 'input' rather than 'stdin'
  // see if fgets() produced error
  if (ferror(input)) 
    {
      perror("fgets() failed");
      exit(1);
    }

  // Close the socket by closing the FILE* wrapper
  fclose(input);
}

