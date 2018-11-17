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
#include "hashTable.h"
/* You will have to modify the program below */

#define MAXBUFSIZE 2048
#define HUGEBUFSIZE 131072
int listening = 1;

void INThandler(int sig);
void handleRequest(int connectionSock, char* request, char* hostName, HashTable* addressCache, char fileName[MAXBUFSIZE]);
int blackList(char* hostname);


int header_read(int server_sock, char *buffer, int buffer_length) {
    int i = 0;
    char ch = '\0';
    int nbytes;
    //Read char by char until end of line
    while (i < (buffer_length - 1) && (ch != '\n')) {
        nbytes = recv(server_sock, &ch, 1, 0);
        if (nbytes > 0) {
            if (ch == '\r') {
                nbytes = recv(server_sock, &ch, 1, MSG_PEEK);
                if ((nbytes > 0) && (ch == '\n')) {
                    recv(server_sock, &ch, 1, 0);
                }
                else {
                    ch = '\n';
                }
            }
            buffer[i] = ch;
            i++;
        }
        else {
            ch = '\n';
        }
    }
    buffer[i] = '\0';
    return i;
}

int server_receive(int server_sock, char *buf) {
    char msgBuffer[HUGEBUFSIZE];
    int contentLength = 0;
    unsigned int offset = 0;
    while (1) {
        int length = header_read(server_sock, msgBuffer, HUGEBUFSIZE);
        if(length <= 0) {
            return -1;
        }

        memcpy((buf + offset), msgBuffer, length);
        offset += length;
        if (strlen(msgBuffer) == 1) {
            break;
        }
        if (strncmp(msgBuffer, "Content-Length", strlen("Content-Length")) == 0) {
            char s1[256];
            sscanf(msgBuffer, "%*s %s", s1);
            contentLength = atoi(s1);
        }
    }
    char* contentBuffer = malloc((contentLength * sizeof(char)) + 3);
    int i;
    for (i = 0; i < contentLength; i++) {
        char c;
        int nbytes = recv(server_sock, &c, 1, 0);
        if (nbytes <= 0) {
            return -1;
        }
        contentBuffer[i] = c;
    }
    contentBuffer[i + 1] = '\r';
    contentBuffer[i + 2] = '\n';
    contentBuffer[i + 3] = '\0';
		printf("Conent Buffer: %s\n", contentBuffer);
    memcpy((buf + offset), contentBuffer, (contentLength + 3));
    return (offset + i + 4);
}

int main(int argc, char* argv[])
{

	int sock;                           //This will be our socket
	int connectionSock;
	struct sockaddr_in server_addr, client_addr;     //"Internet socket address structure"
	struct addrinfo hints, *res, *p;
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	char originalRequest[MAXBUFSIZE];
	char* method;
	char* saveptr;
	char* header;
	char* fullAddress;
	char* protocol;
	char* hostName;

	int pid;
	int timeout;
	char err_500[] = "HTTP/1.1 500 Internal Server Error\r\n\r\n" "Internal Server Error.";
	char err_403[] = "HTTP/1.1 403 Forbidden\r\n\r\n" "ERROR 403 Forbidden.";
	int client_length = sizeof(client_addr);
	HashTable* addressCache = createTable(50);
	// struct timeval tv;
	//
	// typedef struct Page {
	// 	int timeout;
	// 	char* name;
	// 	FILE* fd;
	// } Page;
	//
	// typedef struct PageCache {
	// 	Page** pages;
	// 	int size;
	// } PageCache;
	//
	// Page* tmp = malloc(sizeof(Page));
	// PageCache Book;
	// Book.pages = calloc(10, sizeof(Page));
	// gettimeofday(&tv, NULL);
	// tmp->timeout = tv.tv_sec;
	// printf("Timeout: %i\n", tmp->timeout);
	// sleep(60);
	// gettimeofday(&tv,NULL);
	// printf("Current time: %i\n", tv.tv_sec - tmp->timeout);
	//
	// Book.pages[0] = tmp;
	// printf("First Page: %i\n", Book.pages[0]->timeout);
	// printf("Timeout: %i\n", tmp->timeout);




	/******************
		This code populates the sockaddr_in struct with
		the information about our socket
	 ******************/

	if(argc < 3) {
		printf("USAGE: ./webproxy <port> <timeout>");
		return -1;
	}

	timeout = atoi(argv[2]);

	bzero(&server_addr,sizeof(server_addr));                    //zero the struct
	server_addr.sin_family = AF_INET;                   //address family
	server_addr.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	server_addr.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine

	//Causes the system to create a generic socket of type TCP
	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		printf("unable to create socket");
	}

	int enable = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
		printf("Error on socket options.\n");
		return -1;
	}

	/******************
		Once we've created a socket, we must bind that socket to the
		local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("unable to bind socket\n");
		exit(1);
	}

	if(listen(sock, 5) < 0) {
		printf("unable to listen on port\n");
	}

	bzero(&client_addr, client_length);

	signal(SIGINT, INThandler);
	while(listening) {
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
			close(sock);

			while((nbytes = read(connectionSock, buffer, MAXBUFSIZE)) > 0) {
				printf("REQUEST: %s\n", buffer);

				sprintf(originalRequest, "%s", buffer);
				header = strtok_r(buffer, "\r\n\r\n", &saveptr);
				//printf("HEADER: %s\n", header);
				method = strtok_r(header, " ", &saveptr);
				//printf("METHOD: %s\n", method);

				if(strcmp(method, "GET")) {
          write(connectionSock, err_500, strlen(err_500));
					bzero(buffer,sizeof(buffer));
					continue;
				}

				char tmp[MAXBUFSIZE];
				char fileName[MAXBUFSIZE];
				fullAddress = strtok_r(NULL, " ", &saveptr);
				printf("fullAddress: %s\n", fullAddress);
				sprintf(tmp, "%s", fullAddress);
				protocol = strtok_r(fullAddress, "://", &saveptr);
				//printf("PROTOCOL: %s\n", protocol);
				hostName = strtok_r(NULL, "/", &saveptr);
				//printf("HOST: %s\n", hostName);

				// Get file name
				int fileStart = 0;
				int fileNameIndex = 0;
				for(int i = strlen(tmp) - 1; i >= 0; i--) {
					if(!strncmp(&tmp[strlen(tmp) - 1], "/", 1)) {
						fileStart = -2;
						break;
					}
					if(!strncmp(&tmp[i], "/", 1)) {
						fileStart = i;
						break;
					}
				}
				fileStart++;
				bzero(fileName,sizeof(fileName));

				if(fileStart >= 0) {
					for(int i = fileStart; i < strlen(tmp); i++) {
						if(!strncmp(&tmp[i], "?", 1) || !strncmp(&tmp[i], "\0", 1) || !strncmp(&tmp[i], "\n", 1)) {
							break;
						}
						fileName[fileNameIndex] = tmp[i];
						fileNameIndex++;
					}
				}


				//printf("FileName: %s\n", fileName);

				// TODO: Implement caching
				if(blackList(hostName) == true) {
          write(connectionSock, err_403, strlen(err_403));
					bzero(buffer,sizeof(buffer));
					exit(0);
				}

				handleRequest(connectionSock, originalRequest, hostName, addressCache, fileName);
				bzero(buffer,sizeof(buffer));
			}
			close(connectionSock);
			exit(0);
		}
		// Parent Proccess
		close(connectionSock);
	}
}

void INThandler(int sig)
{
	signal(sig, SIG_IGN);
	listening = 0;
	exit(0);
}

void handleRequest(int connectionSock, char* request, char* hostName, HashTable* addressCache, char* fileName)
{

	int remoteSock, len, nbytes;
	char buffer[MAXBUFSIZE];
	struct hostent* remoteHost;
	struct sockaddr_in remote_addr;
	FILE* fd;

	remoteSock = socket(AF_INET, SOCK_STREAM, 0);

	if(remoteSock < 0) {
		printf("Error opening socket\n");
		exit(-1);
	}

	remoteHost = search(addressCache, hostName);
	if(remoteHost == NULL) {
		printf("MISS\n");
		remoteHost = gethostbyname(hostName);
		if (remoteHost == NULL) {
			 fprintf(stderr,"ERROR, no such host\n");
			 exit(0);
		}
		printf("Caching...\n");
		printf("remoteHost: %s\n", hostName);
		insert(addressCache, hostName, remoteHost);
	}

	bzero((char *) &remote_addr, sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	bcopy((char *)remoteHost->h_addr, (char *)&remote_addr.sin_addr.s_addr, remoteHost->h_length);
	remote_addr.sin_port = htons(80);

	/* Now connect to the server */
	if (connect(remoteSock, (struct sockaddr*)&remote_addr, sizeof(remote_addr)) < 0) {
		 perror("ERROR connecting");
		 exit(-1);
	}

	//Forward Request
	len = write(remoteSock, request, strlen(request));

	// //Receive Response
	// bzero(buffer,sizeof(buffer));
	// while(1) {
	// 	nbytes = read(remoteSock, buffer, MAXBUFSIZE);
	// 	if(nbytes == 0) {
	// 		break;
	// 	}
	// 	printf("DATA: %s\n", buffer);
	// 	//len = send(connectionSock, buffer, nbytes, 0);
	// 	bzero(buffer,sizeof(buffer));
	// }
	// fclose(fd);
	// printf("outside of the loop\n");
	int readBytes;
	if(!strcmp(fileName, "")) {
		printf("hit\n");
		//TODO:check if exsists and send request to server if you do.
		printf("Opening... %s\n", hostName);
		// why is this not working
		fd = fopen("test.txt", "r");
		if(fd == NULL) {
			printf("Error opening file...%s\n", fileName);
			exit(0);
		}
		bzero(buffer,sizeof(buffer));
		while(1) {
			printf("About to READ!\n");
			//readBytes = fread(buffer, sizeof(buffer), 1, fd);
      readBytes = fgetc(fd);
      if( feof(fd) ) {
         break ;
      }
      // printf("%c", c);
			// if(readBytes <= 0) {
			// 	ferror(fd);
			// 	if( ferror(fd) ) {
	    //   printf("Error in reading from file : file.txt\n");
	   	// 	}
			// 	break;
			// }
			// printf("READING: %s %i\n", buffer, readBytes);
			// len = send(connectionSock, buffer, readBytes, 0);
			// bzero(buffer,sizeof(buffer));
		}
	}
	else {
		// works but not for gzip encoding.

		if(!strcmp(fileName, "")) {
			printf("No filename.............\n");
			fd = fopen(hostName, "w+");
			if(fd == NULL) {
				printf("Error opening file...%s\n", fileName);
				exit(0);
			}
		}
		else {
			fd = fopen(fileName, "w+");
			if(fd == NULL) {
				printf("Error opening file...%s\n", fileName);
				exit(0);
			}
		}

		bzero(buffer,sizeof(buffer));
		while(1) {
			nbytes = read(remoteSock, buffer, MAXBUFSIZE);
			if(nbytes == 0) {
				break;
			}
			printf("DATA: %s\n", buffer);
			//readBytes = read(fd, buffer, MAXBUFSIZE);
			fprintf(fd, "%s", buffer);
			len = send(connectionSock, buffer, nbytes, 0);
			bzero(buffer,sizeof(buffer));
		}
	}

	printf("Outside loop....\n");
	fclose(fd);
	close(remoteSock);

}

int blackList(char* hostName)
{

	FILE* fd;
	char line[100];
	char* domain;

	domain = hostName;
	if(!strncmp(hostName, "w", 1)) {
		domain += 4;
	}

	fd = fopen("blacklist.txt", "r");
	if(fd < 0) {
		printf("Error opening file...\n");
		exit(0);
	}

	while(fgets(line, sizeof(line), fd) != NULL) {
		line[strlen(line) - 1] = '\0';
		if(!strcmp(domain, line)) {
			fclose(fd);
			return true;
		}
	}
	fclose(fd);
	return false;

}
