/* A simple server in the internet domain using TCP
   The port number is passed as an argument
   This version runs forever, forking off a separate
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  /* signal name macros, and the kill() prototype */
#include <ctype.h>
#include <dirent.h>

#define BUFFER_MAX 1024
#define HEADER_MAX 128
#define NOT_FOUND_MESSAGE "<!DOCTYPE html><html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>"

char *parseRequest(char *);
char *findMatchingFile(char *);
char *getContentType(char *);
void attachHeader(int, int, char *, int);
void respondRequest(int, char *);

void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    char *part;

    if (argc < 2) {
        fprintf(stderr, "ERROR, should provide a port number and a part\n");
        exit(1);
    }
    else if (argc < 3) {
      	fprintf(stderr, "ERROR, should provide a part\n");
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);  // create socket
    if (sockfd < 0) error("ERROR opening socket");
    memset((char *) &serv_addr, 0, sizeof(serv_addr));  // reset memory

    // fill in address info
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // fill in which part
    part = argv[2];
    printf("PART %s\n\n", part);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    listen(sockfd, 5);  // 5 simultaneous connection at most

    //accept connections
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    if (newsockfd < 0) error("ERROR on accept");

    int n;
    char buffer[BUFFER_MAX];

    memset(buffer, 0, BUFFER_MAX);  // reset memory

    //read client's message
    n = read(newsockfd, buffer, BUFFER_MAX - 1);
    if (n < 0) error("ERROR reading from socket");
    printf("HTTP Request Message: \n%s", buffer);
    
    //reply to client
    if (!strcmp(part, "A")) {
    	n = write(newsockfd, "I got your message", 18);
    	if (n < 0) error("ERROR writing to socket");
    }
    else {
       	char *fileName = parseRequest(buffer);
    	respondRequest(newsockfd, fileName);
    }
	
    close(newsockfd);  // close connection
    close(sockfd);
    return 0;
}

char *parseRequest(char* buffer) {
    char *name;
    const char *delimiter = " ";
    name = strtok(buffer, delimiter);
    name = strtok(NULL, delimiter) + 1;

    // Compute the length of the standard file name
    int len = 0;
    char *p;
    for (p = name; *p != '\0'; p++) {
	if ((*p == '%') && (*(p+1) == '2') && (*(p+2) == '0')) p += 2;
	len++;
    }

    // Fill in the standard file name
    char *fileName = malloc((len + 1) * sizeof(char));
    char *p1, *p2;
    for (p1 = name, p2 = fileName; *p1 != '\0'; p1++) {
	if ((*p1 == '%') && (*(p1+1) == '2') && (*(p1+2) == '0')) {
	    p1 += 2;
	    *p2 = ' ';
	}
	else *p2 = tolower(*p1);
	p2++;
    }
    *p2 = '\0';
    return fileName;
}    

char *findMatchingFile(char *fileName) {
    DIR *dir;
    struct dirent *dirEnt;
    dir = opendir(".");
    if (dir) {
        while ((dirEnt = readdir(dir)) != NULL) {
            char *curFile = dirEnt->d_name;
      	    if (!strcasecmp(fileName, curFile)) return curFile; 
        }
	closedir(dir);
    }
    return fileName;
}
 
char *getContentType(char *fileName) {
    // Here we consider 7 file types: .txt, .html, .htm, .jpg, .jpeg, .gif and binary file
    char *type;
    if (!strstr(fileName, ".")) type = "application/octet-stream";
    else {
        char *last;
    	last = strrchr(fileName, '.');
    	if (!strcmp(last, ".html") || !strcmp(last, ".htm")) type = "text/html";
    	else if (!strcmp(last, ".txt")) type = "text/plain";
    	else if (!strcmp(last, ".jpg") || !strcmp(last, ".jpeg")) type = "image/jpeg";
    	else if (!strcmp(last, ".gif")) type = "image/gif";
        else type = "text/html";
    }
    return type;
}

void attachHeader(int socket, int code, char *contentType, int fileLength) {
    // HTTP Status
    char *status;
    switch(code) {
      	case 200: 
	    status = "HTTP/1.1 200 OK\r\n";
	    break;
    	case 404:
            status = "HTTP/1.1 404 Not Found\r\n";
	    break;
	default:
	    status = "Status type not supported\r\n";
	    break;
    }

    // Connection
    char *connect = "Connection: close\r\n";
    
    // Server
    char *server = "Server: Xiaopei-Hengyu/1.0\r\n";

    // Content-Length
    char len[HEADER_MAX] = "Content-Length: ";
    if (fileLength == -1) strcat(len, "N/A\r\n");
    else {
    	char contentLength[8];
    	sprintf(contentLength, "%d", fileLength); 
    	strcat(len, contentLength);
    	strcat(len, "\r\n");
    }

    // Content-Type
    char type[HEADER_MAX] = "Content-Type: ";
    //char *type = getContentType(fileName);
    strcat(type, contentType);
    strcat(type, "\r\n");

    // Header Buffer
    char headerFields[5][HEADER_MAX];
    strcpy(headerFields[0], status);
    strcpy(headerFields[1], connect);
    strcpy(headerFields[2], server);
    strcpy(headerFields[3], len);
    strcpy(headerFields[4], type);

    // Write header to the client
    char buffer[BUFFER_MAX];
    int i, pos = 0;
    for (i = 0; i < 5; i++) {
	int shift = strlen(headerFields[i]);
	memcpy(buffer + pos, headerFields[i], shift);
	pos += shift;
    }
    memcpy(buffer + pos, "\r\n\0", 3);
    write(socket, buffer, strlen(buffer));
    printf("HTTP Response Message: \n%s", buffer);

    return;
}

void respondRequest(int socket, char *fileName) {
    // File not found
    if (*fileName == '\0') {
	attachHeader(socket, 404, "text/html", -1);
	write(socket, NOT_FOUND_MESSAGE, strlen(NOT_FOUND_MESSAGE));
	return;
    }

    char *realName = findMatchingFile(fileName);
    // Unable to open file
    FILE *fp = fopen(realName, "r");
    if (!fp) {
	attachHeader(socket, 404, "text/html", -1);
	write(socket, NOT_FOUND_MESSAGE, strlen(NOT_FOUND_MESSAGE));
	return;
    }

    fseek(fp, 0L, SEEK_END);
    long fileLength = ftell(fp);
    // Invalid file length
    if (fileLength < 0) {
        attachHeader(socket, 404, "text/html", -1);
	write(socket, NOT_FOUND_MESSAGE, strlen(NOT_FOUND_MESSAGE));
	return;
    }

    // Write file content to the client
    fseek(fp, 0L, SEEK_SET);
    attachHeader(socket, 200, getContentType(fileName), fileLength);
    char buffer[BUFFER_MAX];  
    if (fileLength < BUFFER_MAX) {
	fread(buffer, sizeof(char), fileLength, fp);
        write(socket, buffer, fileLength);
    }
    else {
    	size_t len;
    	while (len = fread(buffer, sizeof(char), BUFFER_MAX, fp)) {
	    write(socket, buffer, (int)len);
    	}
    }

    fclose(fp);
    return;
}

