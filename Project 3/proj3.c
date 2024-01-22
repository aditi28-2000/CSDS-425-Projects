
/* Name: Aditi Mondal
   Case Network ID: axm1849
   filename: proj3.c
   Date created: 9th October, 2023
   Brief Description: This is a simple web server written in C that implements the server half of client-server applications and responds to the client accordingly based on the requests.
 */
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define REQUIRED_ARGC 7
#define ERROR 1
#define QLEN 1
#define PROTOCOL "tcp"
#define BUFLEN 1024
#define MALFORMED_REQUEST "HTTP/1.1 400 Malformed Request\r\n\r\n"
#define UNSUPPORTED_METHOD "HTTP/1.1 405 Unsupported Method\r\n\r\n"
#define PROTOCOL_ERR "HTTP/1.1 501 Protocol Not Implemented\r\n\r\n"
#define SHUTDOWN_RESPONSE "HTTP/1.1 200 Server Shutting Down\r\n\r\n"
#define SHUTDOWN_FORBIDDEN "HTTP/1.1 403 Operation Forbidden\r\n\r\n"
#define FILE_EXISTS_MSSG "HTTP/1.1 200 OK\r\n\r\n"
#define INVALID_FILENAME "HTTP/1.1 406 Invalid Filename\r\n\r\n"
#define FILE_NOT_FOUND "HTTP/1.1 404 File Not Found\r\n\r\n"

unsigned short cmd_line_flags = 0;
char *PORT = NULL;
char *DIRECTORY = NULL;
char *AUTH_TOKEN = NULL;
char method[16], path[128],version[16];

int usage (char *progname)
{
    fprintf (stderr,"usage: %s -n port -d document_directory -a auth_token\n", progname);
    exit (ERROR);
}

int errexit (char *format, char *arg)
{
    fprintf (stderr,format,arg);
    fprintf (stderr,"\n");
    exit (ERROR);
}

void parseargs (int argc, char *argv []) // to parse the command line arguments
{
    int opt;

    while ((opt = getopt (argc, argv, "n:d:a:")) != -1)
    {
        switch (opt)
        {
            case 'n':
              cmd_line_flags=1;
              PORT = optarg;
              break;
            case 'd':
              cmd_line_flags=1;
              DIRECTORY = optarg;
              break;
	    case 'a':
              cmd_line_flags=1;
              AUTH_TOKEN = optarg;
              break; 
            case '?':
            default:
              usage (argv [0]);
        }
    }
    if (cmd_line_flags == 0)
    {
        fprintf (stderr,"error: no command line option given\n");
        usage (argv [0]);
    }
}


int handleHttpRequest(int sd2) // parses the HTTP requests and throws an error for invalid requests
{
    char request[BUFLEN];
    int flag=0;
    memset(request, 0, sizeof(request)); 

    if (read(sd2, request, sizeof(request)) < 0)
      {
      errexit("Error reading from client",NULL);
      }
     //  printf("Received HTTP request:\n%s\n", request);
    if (strstr(request,"\r\n\r\n")!=NULL)  
      {

    // Splitting the request into lines
    char *lines[100]; 
    int numLines = 0;
    char *token = strtok(request, "\r\n");
    lines[0] = token;
    while (token != NULL) {
        numLines++;
        token = strtok(NULL, "\r\n");
    }

    // Parsing the request line
    if (numLines > 0) {

        if (sscanf(lines[0], "%s %s %s", method, path, version) == 3) {
	    
	    // Checking for correct  methods
        if (strcmp(method, "GET") != 0 && strcmp(method, "SHUTDOWN") != 0 && strcmp(method,"POST")!=0 && strcmp(method,"HEAD")!=0 && strcmp(method,"PUT")!=0 && strcmp(method,"DELETE")!=0) {
	  flag=1;
            if (write(sd2, UNSUPPORTED_METHOD, strlen(UNSUPPORTED_METHOD)) < 0) {
                perror("Error writing to client");
            }
        }
        
        // Checking for the presence of "HTTP/" and methods other than GET that have not been implemented in this code
	else if (strncmp(version, "HTTP/", 5) != 0 || strcmp(method,"POST")==0 || strcmp(method,"HEAD")==0 || strcmp(method,"PUT")==0 || strcmp(method,"DELETE")==0){
	  flag=1;
            if (write(sd2, PROTOCOL_ERR, strlen(PROTOCOL_ERR)) < 0) {
                perror("Error writing to client");
            }
        }
        
    }
    }	
    else // in case HTTP request is NULL/ or no HTTP request mentioned on the client side
      {
	 flag=1;
	 if (write(sd2, MALFORMED_REQUEST, strlen(MALFORMED_REQUEST)) < 0) {
            perror("Error writing to client");
          }
      }

      }
    else  // throws a malformed request error if the request does not conform to the above 
      {
	 flag=1;
	 if (write(sd2, MALFORMED_REQUEST, strlen(MALFORMED_REQUEST)) < 0) {
            perror("Error writing to client");
          }
      }
      return flag;
 }

void openFile(int sd2, char *method, char *path) {    // to read and write the contents of the file to the client
  int f=0;
  
    if (strcmp(method,"GET")==0)
      {
	if (path[0]!='/')
	  {
	    f=1;
	   if (write(sd2, INVALID_FILENAME, strlen(INVALID_FILENAME)) < 0) {
            perror("Error writing to client");
            }
	  }
	   else if(path[0]=='/' && path[1]=='\0')
	     path="/homepage.html";
	  
    if(f==0)
      {
    char filePath[256]; 
    snprintf(filePath, sizeof(filePath), "%s%s", DIRECTORY, path);
    FILE *file = fopen(filePath, "rb");

    if (file == NULL) {
      if (write(sd2,FILE_NOT_FOUND, strlen(FILE_NOT_FOUND)) < 0) {
            perror("Error writing to client");
      } 
   
    }

    else {
       char buffer[BUFLEN];
        size_t bytesRead;
        while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {    //reading contents of the file
	  
	  if (write(sd2, FILE_EXISTS_MSSG, strlen(FILE_EXISTS_MSSG)) < 0) {
            perror("Error writing to client");
          }
            if (write(sd2, buffer, bytesRead) < 0) {
               
                perror("Error writing to client");
                break;
            }
        }
        fclose(file);
    } 
  }
}
}

void Shutdown(int sd2, char *method, char *path) // implements the SHUTDOWN protocol
{
  if((strcmp(method,"SHUTDOWN")==0) && (strcmp(path,AUTH_TOKEN)==0))
    {
      if (write(sd2, SHUTDOWN_RESPONSE, strlen(SHUTDOWN_RESPONSE)) < 0) {
            perror("Error writing to client");
      }
      close(sd2);
      exit(0);
    }
  else if((strcmp(method,"SHUTDOWN")==0) && (strcmp(path,AUTH_TOKEN)!=0))  // if the AUTH_TOKEN does not match server throws an error and continues listening on the specified port
    {
      if (write(sd2, SHUTDOWN_FORBIDDEN, strlen(SHUTDOWN_FORBIDDEN)) < 0) {
            perror("Error writing to client");
      }
    }
}


int main (int argc, char *argv [])
{
    parseargs(argc,argv);
    struct sockaddr_in sin;
    struct sockaddr addr;
    struct protoent *protoinfo;
    unsigned int addrlen;
    int sd, sd2, flag=0;
    
    if (argc != REQUIRED_ARGC)
        usage (argv [0]);

    /* determine protocol */
    if ((protoinfo = getprotobyname (PROTOCOL)) == NULL)
        errexit ("cannot find protocol information for %s", PROTOCOL);

    /* setup endpoint info */
    memset ((char *)&sin,0x0,sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons ((u_short) atoi (PORT));

    /* allocate a socket */
    /*   would be SOCK_DGRAM for UDP */
    sd = socket(PF_INET, SOCK_STREAM, protoinfo->p_proto);
    if (sd < 0)
        errexit("cannot create socket", NULL);

    /* bind the socket */
    if (bind (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        errexit ("cannot bind to port %s", PORT);

    /* listen for incoming connections */
    if (listen (sd, QLEN) < 0)
        errexit ("cannot listen on port %s\n",PORT);

    //printf("Server is listening on port %s...\n", PORT);

    while(1)
      {
    /* accept a connection */
    addrlen = sizeof (addr);
    sd2 = accept (sd,&addr,&addrlen);
    if (sd2 < 0)
        errexit ("error accepting connection", NULL);

    flag =handleHttpRequest(sd2);
    if(flag==0)
      {
    openFile(sd2,method,path);
    Shutdown(sd2,method,path);
      }
    close(sd2);
      }
    
    /* write message to the connection 
    if (write (sd2,argv [MSG_POS],strlen (argv [MSG_POS])) < 0)
    errexit ("error writing message: %s", argv [MSG_POS]); */

    /* close connections and exit */
    close (sd);
    exit (0);
}

