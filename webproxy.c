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
int listening = 1;

void INThandler(int sig);
void handleRequest(int connectionSock, char* request, char* hostName, HashTable* addressCache);
int blackList(char* hostname);

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
				printf("BUFFER: %s\n", buffer);

				sprintf(originalRequest, "%s", buffer);
				header = strtok_r(buffer, "\r\n\r\n", &saveptr);
				printf("HEADER: %s\n", header);
				method = strtok_r(header, " ", &saveptr);
				printf("METHOD: %s\n", method);

				if(strcmp(method, "GET")) {
          write(connectionSock, err_500, strlen(err_500));
					bzero(buffer,sizeof(buffer));
					continue;
				}

				fullAddress = strtok_r(NULL, " ", &saveptr);
				printf("fullAddress: %s\n", fullAddress);
				protocol = strtok_r(fullAddress, "://", &saveptr);
				printf("PROTOCOL: %s\n", protocol);
				hostName = strtok_r(NULL, "/", &saveptr);
				printf("HOST: %s\n", hostName);

				// TODO: Implement caching
				if(blackList(hostName) == true) {
          write(connectionSock, err_403, strlen(err_403));
					bzero(buffer,sizeof(buffer));
					exit(0);
				}


				handleRequest(connectionSock, originalRequest, hostName, addressCache);
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

void handleRequest(int connectionSock, char* request, char* hostName, HashTable* addressCache)
{

	int remoteSock, len, nbytes;
	char buffer[MAXBUFSIZE];
	struct hostent* remoteHost;
	struct sockaddr_in remote_addr;

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
	else {
		printf("HIT\n");
		printf("remoteHost: %s\n", hostName);
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

	//Receive Response
	bzero(buffer,sizeof(buffer));
	while(1) {
		nbytes = read(remoteSock, buffer, MAXBUFSIZE);
		if(nbytes == 0) {
			break;
		}
		len = send(connectionSock, buffer, nbytes, 0);
		bzero(buffer,sizeof(buffer));
	}
	printf("outside of the loop\n");
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
