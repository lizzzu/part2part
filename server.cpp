#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>

#include "validation.hpp"

#define MAX_CONNECTIONS 20
#define MAX_THREADS 100

extern int errno;

int createServer(const char* host, int port);
void runServer(const char* host, int port);

char* conv_addr(struct sockaddr_in address);
int sayHello(int fd);

int main(int argc, char* argv[]) {
	char host[20];
	int port;

	getIPandPort(host, port);

	pthread_t th[MAX_THREADS];

	runServer(host, port);

    return 0;
}

int createServer(const char* host, int port) {
	struct sockaddr_in server;
	int sd;

	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("[SERVER] Error: socket()");
		return errno;
	}

	int on = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	bzero(&server, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(host);
	server.sin_port = htons(port);

	if(bind(sd, (struct sockaddr*) &server, sizeof(server)) == -1) {
		perror("[SERVER] Error: bind()");
		return errno;
	}

	return sd;
}

void runServer(const char* host, int port) {
	struct sockaddr_in from;
	bzero(&from, sizeof(from));

	struct timeval tv;

	fd_set readfds;
	fd_set actfds;
	
	int client;
	int fd;
	int nfds;
	unsigned int len;

	int sd = createServer(host, port);
	printf("[SERVER] sd = %d\n", sd);

	if(sd == errno) {
		printf("[SERVER] Cannot create server\n");
		exit(1);
	}

	if(listen(sd, MAX_CONNECTIONS) == -1) {
		perror("[SERVER] Error: listen()");
		exit(EXIT_FAILURE);
	}

	FD_ZERO(&actfds);
	FD_SET(sd, &actfds);

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	
	nfds = sd;

	printf("[SERVER] Waiting on port %d...\n", port);
	fflush(stdout);

	while(1) {
		bcopy((char *) &actfds, (char *) &readfds, sizeof (readfds));

		if(select(nfds + 1, &readfds, NULL, NULL, &tv) < 0) {
			perror("[SERVER] Error: select()\n");
			exit(EXIT_FAILURE);
		}

		if(FD_ISSET(sd, &readfds)) {
			len = sizeof(from);
			bzero(&from, sizeof(from));

			client = accept(sd, (struct sockaddr*) &from, &len);

			if (client < 0) {
				perror("[SERVER] Error: accept()\n");
				continue;
			}

			if (nfds < client)
				nfds = client;
					
			FD_SET(client, &actfds);

			printf("[SERVER] A client has connected: descriptor %d, address %s\n", client, conv_addr(from));
			fflush(stdout);
		}

		for(fd = 0; fd <= nfds; fd++) {
			if (fd != sd && FD_ISSET(fd, &readfds))
				if (sayHello(fd)) {
					printf("[SERVER] The client %d has disconnected\n", fd);
					fflush(stdout);
					close(fd);
					FD_CLR(fd, &actfds);
				}
		}
	}
}

char* conv_addr(struct sockaddr_in address) {
	static char str[25];
	char port[7];

	strcpy(str, inet_ntoa(address.sin_addr));
	bzero(port, 7);

	sprintf(port, ":%d", ntohs(address.sin_port));	
	strcat(str, port);

	return (str);
}

int sayHello(int fd) {
	char buffer[100];
	int bytes;
	char msg[100];
	char msgrasp[100] = " ";

	bytes = read(fd, msg, sizeof(buffer));
	if(bytes < 0) {
		perror("[SERVER] Error: read() from client\n");
		return 0;
	}
	printf("[SERVER] The message has been received: %s\n", msg);
		
	bzero(msgrasp, 100);
	strcat(msgrasp, "Hello ");
	strcat(msgrasp, msg);
	
	printf("[SERVER] Sending back the message... %s\n", msgrasp);
		
	if(bytes && write(fd, msgrasp, bytes) < 0) {
		perror("[SERVER] Error: write() to client.\n");
		return 0;
	}
	
	return bytes;
}
