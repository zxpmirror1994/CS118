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

#define BUFFER_MAX 25
#define HEADER_MAX 128
#define NOT_FOUND_MESSAGE "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY><H1>404 Not Found</H1><P>No requested file or requested file cannot be opened.</P></BODY></HTML>"


char *parseRequest(char *);
int isSameFile(char *, char *);
char *findMatchingFile(char *);
char *getContentType(char *);
void attachHeader(int, int, char *, int);
void respondRequest(int, char *);

int main() {
    char *buffer = "GET /T%20e%20S%220t.html HTTP/1.1";
    char buff[80];
    strcpy(buff, buffer);
    printf("%s\n", parseRequest(buff));
    printf("%d\n", isSameFile("test.txt", "TesT.TxT"));
    printf("%s\n", findMatchingFile("test.txt"));
    printf("%s\n", getContentType("test.jpg"));
    printf("%s\n", getContentType("test.html"));
    printf("%s\n", getContentType("test.htm"));
    printf("%s\n", getContentType("test.gif"));
    printf("%s\n", getContentType("test.jpeg"));
    printf("%s\n", getContentType("test"));
    attachHeader(1, 200, "test.html", 500);
    respondRequest(1, "test.txt");
    return 0;
}

char *parseRequest(char* buffer) {
    char *name;
    const char *delimiter = " ";
    name = strtok(buffer, delimiter);
    name = strtok(NULL, delimiter) + 1;
    int len = 0;
    char *p;
    for (p = name; *p != '\0'; p++) {
	if ((*p == '%') && (*(p+1) == '2') && (*(p+2) == '0')) p += 2;
	len++;
    }

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

int isSameFile(char *f1, char *f2) {
    char *p1, *p2;
    for (p1 = f1, p2 = f2; (*p1 != '\0') && (*p2 != '\0'); p1++, p2++) {
      	if (*p1 != tolower(*p2)) return 0;
    }
    if (*p1 == '\0' && *p2 == '\0') return 1;
    return 0;
}

char *findMatchingFile(char *fileName) {
    DIR *dir;
    struct dirent *dirEnt;
    dir = opendir(".");
    if (dir) {
        while ((dirEnt = readdir(dir)) != NULL) {
            char *curFile = dirEnt->d_name;
      	    if (isSameFile(fileName, curFile)) return curFile; 
        }
	closedir(dir);
    }
    return fileName;
}

char *getContentType(char *fileName) {
    char *type;//, *lower = malloc(strlen(fileName + 1) * sizeof(char));
    /*char *p1, *p2;
    for (p1 = fileName, p2 = lower; *p1 != '\0'; p1++, p2++) {
       	if (*p1 >= 'A' && *p1 <= 'Z') *p2 = tolower(*p1);
        else *p2 = *p1;
    }
    *p2 = '\0';*/

    if (strstr(fileName, ".html") || strstr(fileName, ".htm")) type = "text/html";
    else if (strstr(fileName, ".txt")) type = "text/plain";
    else if (strstr(fileName, ".jpg") || strstr(fileName, ".jpeg")) type = "image/jpeg";
    else if (strstr(fileName, ".gif")) type = "image/gif";
    else type = "application/octet-stream";
    return type;
}

void attachHeader(int socket, int code, char *fileName, int fileLength) {
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
    char contentLength[HEADER_MAX] = "Content-Length: ";
    if (fileLength == -1) strcat(contentLength, "N/A\r\n");
    else {
    	char len[8];
    	sprintf(len, "%d", fileLength); 
    	strcat(contentLength, len);
    	strcat(contentLength, "\r\n");
    }

    // Content-Type
    char contentType[HEADER_MAX] = "Content-Type: ";
    char *type = getContentType(fileName);
    strcat(contentType, type);
    strcat(contentType, "\r\n");

    // Header Buffer
    char headerFields[5][HEADER_MAX];
    strcpy(headerFields[0], status);
    strcpy(headerFields[1], connect);
    strcpy(headerFields[2], server);
    strcpy(headerFields[3], contentLength);
    strcpy(headerFields[4], contentType);

    char buffer[BUFFER_MAX];
    int i, pos = 0;
    for (i = 0; i < 5; i++) {
	int shift = strlen(headerFields[i]);
	memcpy(buffer + pos, headerFields[i], shift);
	pos += shift;
    }
    memcpy(buffer + pos, "\r\n\0", 3);
    //write(socket, buffer, strlen(buffer));
    printf("HTTP Response Message: %s\n", buffer);

    return;
}

void respondRequest(int socket, char *fileName) {
    if (*fileName == '\0') {
	attachHeader(socket, 404, fileName, -1);
	write(socket, NOT_FOUND_MESSAGE, strlen(NOT_FOUND_MESSAGE));
	return;
    }

    char *realName = findMatchingFile(fileName);

    FILE *fp = fopen(realName, "r");
    if (!fp) {
	attachHeader(socket, 404, fileName, -1);
	write(socket, NOT_FOUND_MESSAGE, strlen(NOT_FOUND_MESSAGE));
	return;
    }

    fseek(fp, 0L, SEEK_END);
    long fileLength = ftell(fp);
    if (fileLength < 0) {
        attachHeader(socket, 404, fileName, -1);
	write(socket, NOT_FOUND_MESSAGE, strlen(NOT_FOUND_MESSAGE));
	return;
    }

    fseek(fp, 0L, SEEK_SET);
    attachHeader(socket, 200, fileName, fileLength);
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

