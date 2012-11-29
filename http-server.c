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
#include <sys/types.h>
#include <netdb.h>
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

void HandleTCPClient(int clntSocket, char *web_root, char* IPaddr, int MDBsock);

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

  /* Create socket for mdb-lookup */
  char *mdbserverName = argv[3];   /* fourth arg: mdb host server */
  char *mdbserverIP;
  char *mdbserverPort = argv[4];   /* fifth arg: lookup port */

  int MDBsock;
  struct sockaddr_in mdbserverAddr;
  struct hostent *mdbhe;

  // get server ip from server name
  if ((mdbhe = gethostbyname(mdbserverName)) == NULL)
    die("gethostbyname failed");
  
  mdbserverIP = inet_ntoa(*(struct in_addr *)mdbhe->h_addr);
  
  if((MDBsock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    die("mdb socket() failed");

  //construct server address
  memset(&mdbserverAddr, 0, sizeof(mdbserverAddr));
  mdbserverAddr.sin_family = AF_INET;
  mdbserverAddr.sin_addr.s_addr = inet_addr(mdbserverIP);
  unsigned short mdbport = atoi(mdbserverPort);
  mdbserverAddr.sin_port = htons(mdbport);

  //connect
  if(connect(MDBsock, (struct sockaddr *)&mdbserverAddr, sizeof(mdbserverAddr)) < 0)
      die("connect for mdb failed");

  //PART A

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
      
      HandleTCPClient(clntSock, webroot, clntIPaddr, MDBsock);
    }
  
  /* NOT REACHED */
}

void HandleTCPClient(int clntSocket, char* web_root, char* IPaddr, int MDBsock)
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
      char *method = strtok(requestline, token_separators); // GET
      char *requestURI = strtok(NULL, token_separators);   // /cs3157/tng/
      char *httpVersion = strtok(NULL, token_separators);  // HTTP/1.0

      FILE *read_file;

      char *outtext = IPaddr;
      int sendlength = 0;
      int type = 0;

      // request URI must start with '/'
      // otherwise send 400 Bad Request
      if(method == NULL || requestURI == NULL 
	 || httpVersion == NULL || strncmp("/", requestURI, 1) != 0)
	{ 
	  outtext = "400 Bad Request\n";
	  sendlength = sprintf(out_buffer, "HTTP/1.0 400 Bad Request\n"
		"\n<html><body><h1>400 Bad Request</h1></body></html>\n");
	}
      // only support GET method, must be either HTTP/1.0 or 1.1
      // otherwise send 501 Not Implemented
      else if((strcmp("GET", method) != 0) || 
	 (strcmp("HTTP/1.0", httpVersion) != 0 && 
	  strcmp("HTTP/1.1", httpVersion) != 0))
	{
	  outtext = "501 Not Implemented\n";
	  sendlength = sprintf(out_buffer, "HTTP/1.0 501 Not Implemented\n"
		"\n<html><body><h1>501 Not Implemented</h1></body></html>\n");
	}
      // successful request, send what they ask for
      // mdb-lookup input page
      else if(strcmp(requestURI, "/mdb-lookup") == 0)
	{
	  const char *form = "<html><body><h1>mdb-lookup</h1>\n"
	    "<p>\n"
	    "<form method=GET action=/mdb-lookup>\n"
	    "lookup: <input type=text name=key>\n"
	    "<input type=submit>\n"
	    "</form>\n"
	    "<p></body></html>\n";

	  outtext = "200 OK\n";
	  sendlength = sprintf(out_buffer, "%s", form);
	}
      // mdb-lookup print results page
      else if(strncmp(requestURI, "/mdb-lookup?key", 15) == 0)
	{
	  const char *form = "<html><body><h1>mdb-lookup</h1>\n"
	    "<p>\n"
	    "<form method=GET action=/mdb-lookup>\n"
	    "lookup: <input type=text name=key>\n"
	    "<input type=submit>\n"
	    "</form>\n" "<p>\n</p>\n<p>\n"
	    "<table border>\n"
	    "<tbody>\n";

	  outtext = "200 OK\n";
	  sendlength = sprintf(out_buffer, "%s", form);
	  type = 2;

	}
      else
	{
	  char webfile[strlen(web_root) + strlen(requestURI) + 11];
	  strncpy(webfile, web_root, sizeof(webfile));
	  strcat(webfile, requestURI);
	  // add the index.html if it ends with a /
	  if(requestURI[strlen(requestURI)-1] == '/')
	    { //if it ends in /, append index.html
	      char *end = "index.html";
	      strcat(webfile, end);
	    }
	  read_file = fopen(webfile, "rb");
	  if(read_file == NULL)
	    {
	      outtext = "404 Not Found\n";
	      sendlength = sprintf(out_buffer, "HTTP/1.0 404 Not Found\n"
		 "\n<html><body><h1>404 Not Found</h1></body></html>\n");
	    }
	  // file exists, we will send in - type 1
	  else
	    {
	      outtext = "200 OK\n";
	      sendlength = sprintf(out_buffer, "HTTP/1.0 200 OK\n\n");
	      type = 1;
	    }
    	}
      // log each request to stdout
      fprintf(stdout, "%s \"%s %s %s\" %s", 
	      IPaddr, method, requestURI, httpVersion, outtext);
      fflush(stdout);
      // send the out_buffer
      if(send(clntSocket, out_buffer, sendlength, 0) != sendlength)
	{
	  fclose(input);
	  die("send() has failed");
	}
      // for sending real files (index.html, ship.jpg, crew.jpg)
      if(type == 1)
	{
	  memset(out_buffer, '\0', strlen(out_buffer));
	  while((sendlength = fread(out_buffer, 1, 
				    sizeof(out_buffer), read_file)) > 0)
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
      // for sending mdb-lookup results
      else if(type == 2)
	{
	  char buf[1000];
	  char *key = strstr(requestURI, "=") + 1;
	  // get the key
	  sprintf(buf, "%s\n", key);
	  // send HTTP request
	  if(send(MDBsock, buf, strlen(buf), 0) != strlen(buf))
	    {
	      fclose(input);
	      die("send mdbsock failed");
	    }
	  // open socket to read results
	  FILE* mdbsockfd;
	  if((mdbsockfd = fdopen(MDBsock, "r")) == NULL)
	    {
	      fclose(input);
	      die("fileopen mdb failed");
	    }
	  char buf2[SENDSIZE];
	  memset(buf, '\0', sizeof(buf));
	  while(strcmp(fgets(buf, sizeof(buf), mdbsockfd), "\n") != 0)
	    {
	      //send results
	      sprintf(buf2, "<tr>\n<td>\n %s </td></tr>", buf);
	      if(send(clntSocket, buf2, strlen(buf2), 0) != strlen(buf2))
		{
		  fclose(mdbsockfd);
		  fclose(input);
		  die("send mdb failed");
		}
	    }
	  // close out the table and html sending
	  snprintf(buf, strlen(buf), "</tbody></table></p></body></html>\n");
	  if(send(clntSocket, buf, strlen(buf), 0) != strlen(buf))
	    {
	      fclose(mdbsockfd);
	      fclose(input);
	      die("mdb send fail");
	    }
	  if(ferror(mdbsockfd))
	    {
	      fclose(mdbsockfd);
	      fclose(input);
	      die("mdb fread failed");
	    }
	  fclose(mdbsockfd);
	}
      fclose(input);
    }

}
