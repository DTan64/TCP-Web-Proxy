#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 2048

void INThandler(int sig);

int main(int argc, char* argv[])
{

	int sock;                           //This will be our socket
	int connectionSock;
	struct sockaddr_in server_addr, client_addr;     //"Internet socket address structure"
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	char* method;
	char* splitInput;
	char fileName[MAXBUFSIZE];
	char folderName[MAXBUFSIZE];
	char filePath[MAXBUFSIZE];
	char fileType[MAXBUFSIZE];
	char uri[MAXBUFSIZE];
	int fd;
	int len;
	int i;
	int readBytes;
	int typeIndex;
	struct stat st;
	int pid;
	char file_size[MAXBUFSIZE];
	char sendBuffer[MAXBUFSIZE];
	char png_response[] = "HTTP/1.1 200 OK\r\n" "Content-Type: image/png; charset=UTF-8\r\n";
	char gif_response[] = "HTTP/1.1 200 OK\r\n" "Content-Type: image/gif; charset=UTF-8\r\n";
	char html_response[] = "HTTP/1.1 200 OK\r\n" "Content-Type: text/html; charset=UTF-8\r\n";
	char text_response[] = "HTTP/1.1 200 OK\r\n" "Content-Type: text/plain; charset=UTF-8\r\n";
	char css_response[] = "HTTP/1.1 200 OK\r\n" "Content-Type: text/css; charset=UTF-8\r\n";
	char jpg_response[] = "HTTP/1.1 200 OK\r\n" "Content-Type: image/jpeg; charset=UTF-8\r\n";
	char js_response[] = "HTTP/1.1 200 OK\r\n" "Content-Type: text/javascript; charset=UTF-8\r\n";
	char htm_response[] = "HTTP/1.1 200 OK\r\n" "Content-Type: text/html; charset=UTF-8\r\n";
	char err_response[] = "HTTP/1.1 500 Internal Server Error\r\n\r\n" "Internal Server Error.";
	char contentLenght[] = "Content-Length: ";
	int client_length = sizeof(client_addr);

	/******************
		This code populates the sockaddr_in struct with
		the information about our socket
	 ******************/

	if(argc < 2) {
		printf("No port specified\n");
		return -1;
	}

	bzero(&server_addr,sizeof(server_addr));                    //zero the struct
	server_addr.sin_family = AF_INET;                   //address family
	server_addr.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

	//Causes the system to create a generic socket of type TCP
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
		printf("unable to create socket");
	}

	//fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
	/******************
		Once we've created a socket, we must bind that socket to the
		local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		printf("unable to bind socket\n");
		exit(1);
	}

	if(listen(sock, 5) < 0) {
		printf("unable to listen on port\n");
	}

	bzero(&client_addr, client_length);


	signal(SIGINT, INThandler);
	while(1) {
		// Accept Connection
		connectionSock = accept(sock, (struct sockaddr *)&client_addr, (socklen_t *) &client_length);
		if(connectionSock < 0) {
				perror("ERROR on accept");
				exit(1);
		}

		pid = fork();
		if (pid < 0) {
				perror("ERROR on fork");
				exit(1);
		}

		// Spawn child process for multiple connections.
		if (pid == 0) {
			bzero(buffer,sizeof(buffer));
			bzero(fileName,sizeof(fileName));
			bzero(folderName,sizeof(folderName));
			bzero(filePath,sizeof(filePath));
			bzero(fileType,sizeof(fileType));
			bzero(uri,sizeof(uri));

			nbytes = read(connectionSock, buffer, MAXBUFSIZE);
			if(nbytes < 0) {
				perror("ERROR reading from socket");
				exit(1);
			}

			// GET or POST
			method = strtok(buffer, " ");
			if(strcmp(method, "GET")) {
			 len = send(connectionSock, err_response, strlen(err_response), 0);
			 exit(0);
			}

			// Get URI
			splitInput = strtok(NULL, " ");
			sprintf(uri, "%s", splitInput);

			if(!strcmp(splitInput, "/") || !strcmp(splitInput, "/index.html")) {

			 bzero(sendBuffer,sizeof(sendBuffer));
			 strcat(sendBuffer, html_response);
			 strcat(sendBuffer, contentLenght);
			 if(stat("index.html", &st) == 0)
					 sprintf(file_size, "%lli", st.st_size);
			 strcat(sendBuffer, file_size);
			 strcat(sendBuffer, "\r\n\r\n");
			 len = send(connectionSock, sendBuffer, strlen(sendBuffer), 0);
			 bzero(buffer,sizeof(buffer));
			 fd = open("index.html", O_RDONLY);
			 while(1) {
				 readBytes = read(fd, buffer, sizeof(buffer));
				 if(readBytes == 0) {
					 break;
				 }
				 len = send(connectionSock, buffer, readBytes, 0);
				 bzero(buffer,sizeof(buffer));
			 }
			 close(fd);

			}
			else {
				int uriLen = strlen(uri);
				if (splitInput[0] == '/') splitInput++;
				sprintf(filePath, "%s", splitInput);
				splitInput = strtok(filePath, "/");
				sprintf(folderName, "%s", splitInput);
				splitInput = strtok(NULL, "/");
				sprintf(fileName, "%s", splitInput);
				for(i = uriLen; i >= 0; i--) {
					if(!strncmp(&uri[i], ".", 1)) {
						typeIndex = i;
						break;
					}
				}

				// Get filePath
				sprintf(fileType, "%s", &uri[typeIndex + 1]);
				bzero(filePath,sizeof(filePath));
				strcat(filePath, ".");
				strcat(filePath, uri);

				// Check if file exists.
				if (stat(filePath, &st) == -1) {
					len = send(connectionSock, err_response, strlen(err_response), 0);
					exit(0);
				}

				if(!strcmp(fileType, "png")) {
					bzero(sendBuffer,sizeof(sendBuffer));
					strcat(sendBuffer, png_response);
					strcat(sendBuffer, contentLenght);
					if (stat(filePath, &st) == 0) {
					 sprintf(file_size, "%lli", st.st_size);
					}
					strcat(sendBuffer, file_size);
					strcat(sendBuffer, "\r\n\r\n");
					len = send(connectionSock, sendBuffer, strlen(sendBuffer), 0);
				}
				else if(!strcmp(fileType, "gif")) {
					bzero(sendBuffer,sizeof(sendBuffer));
					strcat(sendBuffer, gif_response);
					strcat(sendBuffer, contentLenght);
					if (stat(filePath, &st) == 0) {
						sprintf(file_size, "%lli", st.st_size);
					}
					strcat(sendBuffer, file_size);
					strcat(sendBuffer, "\r\n\r\n");
					len = send(connectionSock, sendBuffer, strlen(sendBuffer), 0);
				}
				else if(!strcmp(fileType, "html")) {
					bzero(sendBuffer,sizeof(sendBuffer));
					strcat(sendBuffer, html_response);
					strcat(sendBuffer, contentLenght);
					if (stat(filePath, &st) == 0) {
					 sprintf(file_size, "%lli", st.st_size);
					}
					strcat(sendBuffer, file_size);
					strcat(sendBuffer, "\r\n\r\n");
					len = send(connectionSock, sendBuffer, strlen(sendBuffer), 0);
				}
				else if(!strcmp(fileType, "txt")) {
					bzero(sendBuffer,sizeof(sendBuffer));
					strcat(sendBuffer, text_response);
					strcat(sendBuffer, contentLenght);
					if (stat(filePath, &st) == 0) {
					 sprintf(file_size, "%lli", st.st_size);
					}
					strcat(sendBuffer, file_size);
					strcat(sendBuffer, "\r\n\r\n");
					len = send(connectionSock, sendBuffer, strlen(sendBuffer), 0);
				}
				else if(!strcmp(fileType, "css")) {
					bzero(sendBuffer,sizeof(sendBuffer));
					strcat(sendBuffer, css_response);
					strcat(sendBuffer, contentLenght);
					if (stat(filePath, &st) == 0) {
					 sprintf(file_size, "%lli", st.st_size);
					}
					strcat(sendBuffer, file_size);
					strcat(sendBuffer, "\r\n\r\n");
					len = send(connectionSock, sendBuffer, strlen(sendBuffer), 0);
				}
				else if(!strcmp(fileType, "jpg")) {
					bzero(sendBuffer,sizeof(sendBuffer));
					strcat(sendBuffer, jpg_response);
					strcat(sendBuffer, contentLenght);
					if (stat(filePath, &st) == 0) {
					 sprintf(file_size, "%lli", st.st_size);
					}
					strcat(sendBuffer, file_size);
					strcat(sendBuffer, "\r\n\r\n");
					len = send(connectionSock, sendBuffer, strlen(sendBuffer), 0);
				}
				else if(!strcmp(fileType, "js")) {
					bzero(sendBuffer,sizeof(sendBuffer));
					strcat(sendBuffer, js_response);
					strcat(sendBuffer, contentLenght);
					if (stat(filePath, &st) == 0) {
					 sprintf(file_size, "%lli", st.st_size);
					}
					strcat(sendBuffer, file_size);
					strcat(sendBuffer, "\r\n\r\n");
					len = send(connectionSock, sendBuffer, strlen(sendBuffer), 0);
				}
				else if(!strcmp(fileType, "htm")) {
					bzero(sendBuffer,sizeof(sendBuffer));
					strcat(sendBuffer, htm_response);
					strcat(sendBuffer, contentLenght);
					if (stat(filePath, &st) == 0) {
					sprintf(file_size, "%lli", st.st_size);
					}
					strcat(sendBuffer, file_size);
					strcat(sendBuffer, "\r\n\r\n");
					len = send(connectionSock, sendBuffer, strlen(sendBuffer), 0);
				}

				fd = open(filePath, O_RDONLY);
				if(fd < 0) {
					printf("Error opening file.\n");
					exit(0);
				}

				bzero(buffer,sizeof(buffer));
				while(1) {
					readBytes = read(fd, buffer, sizeof(buffer));
					if(readBytes == 0) {
					 break;
					}
					len = send(connectionSock, buffer, readBytes, 0);
					bzero(buffer,sizeof(buffer));
				}
				close(fd);
			}
			exit(0);
		}
		// Parent Proccess
		close(connectionSock);
	}
}

void INThandler(int sig)
{
	signal(sig, SIG_IGN);
	exit(0);
}
