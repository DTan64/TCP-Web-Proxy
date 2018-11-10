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
int listening = 1;

void INThandler(int sig);

int main(int argc, char* argv[])
{

	int sock, clientSock;                           //This will be our socket
	int connectionSock;
	struct sockaddr_in server_addr, client_addr, remote_addr;     //"Internet socket address structure"

	struct addrinfo hints, *res, *p;
	int status;
  char ipstr[INET6_ADDRSTRLEN];
	struct hostent *remoteHost;

	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	char originalRequest[MAXBUFSIZE];
	char* method;
	char* splitInput;
	char* dns;

	char* saveptr;
	char* header;
	char* fullAddress;
	char* protocol;
	char* hostName;

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

			sprintf(originalRequest, "%s", buffer);
			header = strtok_r(buffer, "\r\n\r\n", &saveptr);
			printf("HEADER: %s\n", header);
			method = strtok_r(header, " ", &saveptr);
			printf("METHOD: %s\n", method);
			fullAddress = strtok_r(NULL, " ", &saveptr);
			printf("fullAddress: %s\n", fullAddress);
			protocol = strtok_r(fullAddress, "://", &saveptr);
			printf("PROTOCOL: %s\n", protocol);
			hostName = strtok_r(NULL, "/", &saveptr);
			printf("HOST: %s\n", hostName);


	    // int result = getaddrinfo(hostName, "80", &hints, &infoptr);
	    // if (result) {
	    //     fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
	    //     exit(1);
	    // }
			//
	    // char ipAddress[MAXBUFSIZE];
			// getnameinfo(infoptr->ai_addr, infoptr->ai_addrlen, ipAddress, sizeof (ipAddress), NULL, 0, NI_NUMERICHOST);
			// printf("IP: %s\n", ipAddress);


			memset(&hints, 0, sizeof hints);
		  hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
		  hints.ai_socktype = SOCK_STREAM;

		  if ((status = getaddrinfo(hostName, NULL, &hints, &res)) != 0) {
		      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
		      return 2;
		  }

		  // printf("IP addresses for %s:\n\n", hostName);
			//
			// void *addr;
			// char *ipver;
		  // for(p = res;p != NULL; p = p->ai_next) {
			//
			//
		  //     // get the pointer to the address itself,
		  //     // different fields in IPv4 and IPv6:
		  //     if (p->ai_family == AF_INET) { // IPv4
		  //         struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
		  //         addr = &(ipv4->sin_addr);
		  //         ipver = "IPv4";
			// 				break;
		  //     } else { // IPv6
		  //         struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
		  //         addr = &(ipv6->sin6_addr);
		  //         ipver = "IPv6";
		  //     }
			//
		  //     // convert the IP to a string and print it:
		  //     inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
		  //     printf("  %s: %s\n", ipver, ipstr);
		  // }
			//
			// res = p;
			// printf("out of loop(P): \n");
			// inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
			// printf("  %s: %s\n", ipver, ipstr);
			// printf("out of loop(Res): \n");
			// inet_ntop(res->ai_family, addr, ipstr, sizeof ipstr);
			// printf("  %s: %s\n", ipver, ipstr);

		  //freeaddrinfo(res); // free the linked list
			//printf("IP: %s\n", host);


	    //freeaddrinfo(infoptr);





			// bzero(&remote,sizeof(remote));               //zero the struct
			// remote.sin_family = AF_INET;
			// remote.sin_addr.s_addr = inet_addr(host);

			if ((clientSock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
      	printf("unable to create socket");
				exit(-1);
  		}

			// if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
      // perror("ERROR connecting");
      // exit(1);
   		// }

			// if(connect(clientSock, (struct sockaddr*) &remote, sizeof(remote)) < 0) {
			// 	printf("unable to connect\n");
			// 	exit(-1);
			// }
			//
			// printf("CONNECTED\n");

			// printf("%s\n",host);
			if(connect(clientSock, res->ai_addr, res->ai_addrlen) != -1)
			{
				// convert the IP to a string and print it:
				char ipstr[INET6_ADDRSTRLEN];
				void *addr;
				if (res->ai_family == AF_INET)
				{ // IPv4
					struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;
					addr = &(ipv4->sin_addr);
				}
				else
				{ // IPv6
					struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)res->ai_addr;
					addr = &(ipv6->sin6_addr);
				}

				inet_ntop(res->ai_family, addr, ipstr, sizeof(ipstr));
			}
			else
			{
				shutdown(clientSock,2);
				printf("UNABLE CONNECT TO: %s\n", hostName);
				perror("client: connect");
				return -1;
			}

			freeaddrinfo(res);
			printf("CONNECTED TO: %s\n", hostName);

			// if(connect(clientSock, res->ai_addr, res->ai_addrlen) < 0) {
			// 	printf("unable to connect to: %s\n", hostName);
			// 	exit(-1);
			// }
			// printf("CONNECTED TO: %s\n", hostName);

			// //Forward request
			// len = write(clientSock, originalRequest, strlen(originalRequest));
			// printf("Bytes sent: %i\n", len);
			//
			// bzero(buffer,sizeof(buffer));
			// while(1) {
			// 	nbytes = read(clientSock, buffer, MAXBUFSIZE);
			// 	if(nbytes < 0) {
			// 		break;
			// 	}
			// 	printf("BUFFER: %s\n", buffer);
			// }







			//
			// //Forward request
			// len = write(clientSock, originalRequest, strlen(originalRequest));
			// printf("Bytes sent: %i\n", len);
			//
			// bzero(buffer,sizeof(buffer));
			// while(1) {
			// 	nbytes = read(clientSock, buffer, MAXBUFSIZE);
			// 	if(nbytes < 0) {
			// 		break;
			// 	}
			// 	printf("BUFFER: %s\n", buffer);
			// }















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
