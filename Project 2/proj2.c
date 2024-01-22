
/* Name : Aditi Mondal
   Case Network ID : axm1849
   Filename : proj2.c
   Date Created : 15th September, 2023
   Brief Description: It is a simple client/server application in C that exchanges information with another computer over a network and returns the output according to the arguments passed. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <unistd.h>

// Using #define to create macro definitions

#define Missing_Args_ERR "Compulsory arguments missing!\n"
#define HTTP_ERR "HTTP Version not Supported\n"
#define Error 1
#define Required_Args 5
#define Response_Code_ERR "ERROR: non-200 response code\n"
#define Invalid_URL "The entered URL is not valid!\n"
#define File_RW_ERR "Error reading/ writing to a file!\n"
#define Protocol "tcp"
#define Buffer_Capacity 1024       // size of the buffer
#define Request_Type "GET "
#define Server_Info "Host: "
#define Carriage_Return "\r\n"
#define HTTP_Version " HTTP/1.0\r\n"
#define User_Agent "User-Agent: CWRU CSDS 325 SimpleClient 1.0\r\n"


// Invalid command syntax
int cmdUsage(char *usage) {
    fprintf(stderr, "usage: %s -u URL [-d] [-q] [-r] [-f] -o filename\n", usage);
    exit(Error);
}


// Function to save web data to a file
int saveWebDataToFile(char *filePath, char *webData) {
    int success = 0; // Initializing the success flag
    FILE *filePointer = fopen(filePath, "w+");

    if (filePointer != NULL) {
        
        char *dataBody = strstr(webData, "\r\n\r\n");

        if (dataBody != NULL) {
            // Writing the data to the file, skipping the initial 4 characters
            if (fputs(dataBody + 4, filePointer) != EOF) {
                success = 1; // Set success flag to 1
            }
        }

        // Closing the file
        fclose(filePointer);
    }

    return success; // Returning the success status
}

int customError(char *message, char *arg) {
  fprintf(stderr, message, arg, "\n");
  exit(Error);
}

int main(int argc, char *argv[])
{
    struct sockaddr_in sin;
    struct hostent *hostInfo;
    struct protoent *protocolInfo;
    
    int socketDescriptor, returnCode, option, sendRequest;
    bool flag=true;
    char *URL = NULL;                // Stores the entered URL
    bool URLInfo = false;            // to check if URL exists after '-u'
    bool fileNameInfo = false;       // to check if filename exists after -o 
    bool DBGInfo = false;            // to check if user enters -d
    bool HTTPRequestInfo = false;    // to check if user enters -q
    bool HTTPResponseInfo = false;   // to check if user enters -r
    bool Redirects = true;           // to check if user enters -f
    char *outputFile = NULL;         // Perform write operation to a file locally
    bool serverStatus = false;       // to check if response from the server is "200 OK"


    if (argc < Required_Args) // check for mandatory number of arguments
      {
      cmdUsage(argv[0]);
      exit(Error);
      }

    while (optind < argc) {

      option = getopt(argc, argv, ":u:o:dqrf");
    
    // To disable automatic error printing
    if (option == -1) {
        optind += 1;
        continue;
    }
    switch (option) {
        case 'u':
            URLInfo = true;
            URL = optarg;
            break;
        case 'o':
            fileNameInfo = true;
            outputFile = optarg;
            break;
        case 'd':
            DBGInfo = true;
            break;
        case 'q':
            HTTPRequestInfo = true;
            break;
        case 'r':
            HTTPResponseInfo = true;
            break;
        case 'f':
	    Redirects = true;
	    break;
        case '?':
            printf("Invalid option: %c\n", option);
            break;
        case ':':
            printf("No argument for %c\n", option);
            cmdUsage(argv[0]);
            break;
    }
   }
   
    if (!URLInfo || !fileNameInfo) // checking if the mandatory arguments exist
      {
	printf(Missing_Args_ERR);
	cmdUsage(argv[0]);
      }
    while (flag)
      {
    for(int i=0; i< strlen(URL);i++)
      {
	URL[i] = tolower(URL[i]); //converted URL to lowercase for easy parsing
      }
    char *URLCopy = malloc(strlen(URL) +1);
    strcpy(URLCopy, URL); // stored a copy of the URL entered in URLCopy

    char *URLParsed = strtok(URL, "/");
    if ((strcmp(URLParsed, "http:"))!=0)
      {
	printf(HTTP_ERR);
	exit(1);
      }
    
    /* Parsing the URL*/
    char **token = malloc(sizeof(char *) * Buffer_Capacity);
    int j=0;
    for (j=0; URLParsed != NULL; j++)
      {
        token[j] = URLParsed;
        URLParsed = strtok(NULL, "/");
    }

    if (j<2) // to check the minimum number of tokens in a URL
      {
	printf(Invalid_URL);
	cmdUsage(argv[0]);
      }

    // Storing hostname and Web File name 
    char *Host = malloc(strlen(token[1]) + 1);  
    char *WebFile = malloc(2 * j);            

    memcpy(Host, token[1], strlen(token[1]));

    if (j == 2)
      {
        strcat(WebFile, "/");
      }
    else
      {
	int k=2;
	while(k<j)
	  {
            strcat(WebFile, "/");
            strcat(WebFile, token[k]);
	    k++;
          }
        if (strcmp(&URLCopy[strlen(URLCopy)-1], "/") == 0)
	  {
            // last char is a /
            strcat(WebFile, "/");
        }
    }

    //  when -d is given, print debugging information
    if (DBGInfo) {
        printf("DBG: host:  %s\n", Host);
        printf("DBG: web_file:  %s\n", WebFile);
        printf("DBG: output_file:  %s\n", outputFile);
	flag=false;
    }

    size_t requestTypeLen = strlen(Request_Type);
    size_t webFileLen = strlen(WebFile);
    size_t httpVersionLen = strlen(HTTP_Version);
    size_t serverInfoLen = strlen(Server_Info);
    size_t hostLen = strlen(Host);
    size_t carriageReturnLen = 2*strlen(Carriage_Return);
    size_t userAgentLen = strlen(User_Agent);

    int reqLength = requestTypeLen + webFileLen + httpVersionLen + serverInfoLen + hostLen + carriageReturnLen + userAgentLen;
    char *request = malloc(reqLength + 1);
    strcat(strcat(strcat(strcat(strcat(strcat(strcat(strcat(request, Request_Type), WebFile), HTTP_Version), Server_Info), Host), Carriage_Return), User_Agent), Carriage_Return);

    // when -q is given, printing the HTTP request sent by web client to the web server
    if (HTTPRequestInfo) {
        char copy_request[reqLength + 1];
        strcpy(copy_request, request);
        char *command;
        command = strtok(copy_request, "\r\n");
        while (command != NULL) {
            printf("OUT: %s\n", command);
            command = strtok(NULL, "\r\n");
        }
	 flag=false;
    }

    hostInfo= gethostbyname(Host); // pointer to a structure that will hold information about the host, including its IP address
    if (hostInfo == NULL)
      {
	printf("Unable to reach: %s", Host);
      }

    
    // establishing connection to the host IP
    memset((char *)&sin, 0x0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(80); // Port position:80
    memcpy((char *)&sin.sin_addr, hostInfo->h_addr, hostInfo->h_length);

   if ((protocolInfo = getprotobyname(Protocol)) == NULL)
        customError("Unable to find protocol for %s", Protocol);
    
    // Creating a TCP socket for IPv4 communication
    socketDescriptor = socket(PF_INET, SOCK_STREAM, protocolInfo->p_proto);
    if (socketDescriptor < 0) {
        customError("Unable to establish socket",NULL);
    }

    connect(socketDescriptor,(struct sockaddr *)&sin, sizeof(sin));
       

    sendRequest = send(socketDescriptor, request, strlen(request), 0);
    if (sendRequest == -1) {
      printf("Request not sent");   // when there is error in sending HTTP GET Request
    }

    char *responseBuffer=malloc(Buffer_Capacity);
    memset(responseBuffer,0x0,Buffer_Capacity);
    size_t newSpace = Buffer_Capacity;

    int bytes = 0;  // to keep track of the number of bytes read
    while ((returnCode = read(socketDescriptor, responseBuffer + bytes, Buffer_Capacity - 1)) > 0)
      {
        bytes=bytes + returnCode;
        newSpace=newSpace + Buffer_Capacity;
        char *tempMemory = realloc(responseBuffer, newSpace);
        if (tempMemory != NULL)
	  {
            responseBuffer = tempMemory;
        }

	else
	  {
            printf("Not enough Memory\n");
            exit(1);
        }
    }
    if (returnCode < 0)
      customError("Error while reading data", NULL);
    
    char *responseBufferPointer = responseBuffer;
    char *responseHeaderBodyPointer = strstr(responseBuffer, "\r\n\r\n");
    char *copyResponseHeader = malloc((responseHeaderBodyPointer - responseBufferPointer + 1) * sizeof(char));
    memset(copyResponseHeader, 0x0, (responseHeaderBodyPointer - responseBufferPointer + 1));
    memcpy(copyResponseHeader, responseBuffer, (responseHeaderBodyPointer - responseBufferPointer));
    char *parsedResponseHeader = strtok(copyResponseHeader, "\r\n");

    if (Redirects) // when -f is given, follow redirects from server
      {
	if (strstr(parsedResponseHeader, "301 Moved Permanently") != NULL)
	    serverStatus = true;
	char *locationHeader= NULL;
	
	if(HTTPResponseInfo)
	  {
         while (parsedResponseHeader != NULL) {
	   
	   if (strstr(parsedResponseHeader, "Location: ") != NULL) // check if Location exists in the HTTP header
	   locationHeader = strstr(parsedResponseHeader, "Location: "); 

	   printf("INC: %s\n", parsedResponseHeader);
	   	
            parsedResponseHeader = strtok(NULL, "\r\n");
        }
	 
        if (locationHeader != NULL) {
        // Extract the new URL from the Location header
	  URL = locationHeader + strlen("Location: ");
	  flag=true;
	    }
	else
	  flag=false;
	  
      }
     }
      
    else if (HTTPResponseInfo) // when -r is given printing the HTTP response header received
      {
      while (parsedResponseHeader != NULL) {
            printf("INC: %s\n", parsedResponseHeader);
            parsedResponseHeader = strtok(NULL, "\r\n");
        }
      flag=false;
      /* if (parsedResponseHeader == NULL)
	 printf("could not pasrse response heade");*/
     }
      }
    
    if (serverStatus == false && Redirects==false)
      {
	printf(Response_Code_ERR);
      }
    flag=false;

    /*else {
        // error handling for -o
        int response = saveWebDataToFile(outputFile, responseBuffer);
        if (response == 0) {
            printf(File_RW_ERR);
            exit(1);
        }
	}*/
   
    
    return 0;
}


    
 



 
