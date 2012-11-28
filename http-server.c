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

void HandleTCPClient(int clntSocket, char *web_root, char* IPaddr);

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
  char *webroot = argv[2];  /* third arg: root */

  char *mdb_host = argv[3];  /*fourth arg: host path */
  unsigned short mdb_port = atoi(argv[4]);  /* fifth arg: lookup port */

  /* Create socket for mdb-lookup */



  // if((clntSocket


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
      
      char *clntIPaddr = inet_ntoa(clntAddr.sin_addr);
      
      HandleTCPClient(clntSock, webroot, clntIPaddr);
    }
  /* NOT REACHED */
}

void HandleTCPClient(int clntSocket, char* web_root, char* IPaddr)
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
      // parse the first line
      char *method = strtok(requestline, token_separators);
      char *requestURI = strtok(NULL, token_separators);
      char *httpVersion = strtok(NULL, token_separators);

      FILE *read_file;

      char *outtext = IPaddr;
      int sendlength = 0;
      int type = 0;

      if(method == NULL || requestURI == NULL 
	 || httpVersion == NULL || strncmp("/", requestURI, 1) != 0)
	{ // request URI must start with /
	  // otherwise send 400 bad request
	  outtext = "400 Bad Request\n", 
	    IPaddr, method, requestURI, httpVersion;
	  sendlength = sprintf(out_buffer, "HTTP/1.0 400 Bad Request\n"
		"\n<html><body><h1>400 Bad Request</h1></body></html>\n");
	}
      else if((strcmp("GET", method) != 0) || 
	 (strcmp("HTTP/1.0", httpVersion) != 0 && 
	  strcmp("HTTP/1.1", httpVersion) != 0))
	{ // only support GET method, must be either HTTP/1.0 or 1.1
	  // otherwise send 501 not implemented
	  outtext = "501 Not Implemented\n", 
	    IPaddr, method, requestURI, httpVersion;
	  sendlength = sprintf(out_buffer, "HTTP/1.0 501 Not Implemented\n"
		"\n<html><body><h1>501 Not Implemented</h1></body></html>\n");
	}
      // successful request, send what they ask for
      else if(strcmp(requestURI, "/mdb-lookup") == 0)
	{
	  const char *form = "<html><body><h1>mdb-lookup</h1>\n"
	    "<p>\n"
	    "<form method=GET action=/mdb-lookup>\n"
	    "lookup: <input type=text name=key>\n"
	    "<input type=submit>\n"
	    "</form>\n"
	    "<p></body></html>\n";

	  outtext = "200 OK\n", 
	    IPaddr, method, requestURI, httpVersion;
	  sendlength = sprintf(out_buffer, "%s", form);
	}
      else
	{
	  char webfile[strlen(web_root) + strlen(requestURI) + 11];
	  strncpy(webfile, web_root, sizeof(webfile));
	  strcat(webfile, requestURI);
	  if(requestURI[strlen(requestURI)-1] == '/')
	    { //if it ends in /, append index.html
	      char *end = "index.html";
	      strcat(webfile, end);
	    }
	  read_file = fopen(webfile, "rb");
	  if(read_file == NULL)
	    {
	      outtext = "404 Not Found\n", 
		IPaddr, method, requestURI, httpVersion;
	      sendlength = sprintf(out_buffer, "HTTP/1.0 404 Not Found\n"
		 "\n<html><body><h1>404 Not Found</h1></body></html>\n");
	    }
	  else
	    {
	      outtext = "200 OK\n", 
		IPaddr, method, requestURI, httpVersion;
	      sendlength = sprintf(out_buffer, "HTTP/1.0 200 OK\n\n");
	      type = 1;
	    }
    	}

      fprintf(stdout, "%s \"%s %s %s\" %s", 
	      IPaddr, method, requestURI, httpVersion, outtext);
      fflush(stdout);
    
      if(send(clntSocket, out_buffer, sendlength, 0) != sendlength)
	{
	  fclose(input);
	  die("send() has failed");
	}
    
      if(type == 1)
	{
	  memset(out_buffer, '\0', strlen(out_buffer));
	  while((sendlength = fread(out_buffer, 1, sizeof(out_buffer), read_file)) > 0)
	    {
	      if(ferror(input))
		{
		  perror("send failed");
		  fclose(input);
		  fclose(read_file);
		  exit(1);
		}
	      if(send(clntSocket, out_buffer, sendlength, 0) != sendlength)
		{
		  fclose(input);
		  fclose(read_file);
		  die("send() failed");
		}
	    }
	  fclose(read_file);
	}
      fclose(input);
    }

}
