#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "validation.hpp"

#define LOCALHOST "127.0.0.1"

extern int errno;

int initPeer(const char* host, int port);
void sendRequest(const char* host, int port);

void searchFile();
void downloadFile();
void removeFile();

int main(int argc, char* argv[]) {
    printf("******** Welcome! ********\n");

    char host[20];
	char p[20];

	// IP
	printf("Enter server IP (for localhost, enter 0):\n> ");
	scanf("%s", host);

	while(!validateIPaddr(host)) {
		printf("Invalid IP address. Try again:\n> ");
		scanf("%s", host);
	}
	if(strlen(host) == 1 && host[0] == '0')
		strcpy(host, LOCALHOST);

	// port
	printf("Enter port number:\n> ");
	scanf("%s", p);

	while(!validatePort(p)) {
		printf("Invalid port number. Try again:\n> ");
		scanf("%s", p);
	}
	uint16_t port = atoi(p);
	
	printf("-----------------------------------\n");
	printf("IP address: %s || Port: %d\n", host, port);
	printf("-----------------------------------\n");
    
    sendRequest(host, port);
    
    return 0;
}

int initPeer(const char* host, int port) {
    struct sockaddr_in server;
    int sd;

    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[PEER] Error: socket()");
		return errno;
    }

    printf("[PEER] sd = %d\n", sd);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_port = htons(port);

    if(connect(sd, (struct sockaddr*) &server, sizeof(server))) {
        close(sd);
        perror("[PEER] Error: connect()\n");
        return errno;
    }

    return sd;
}

void sendRequest(const char* host, int port) {
    char msg[100];

    int sd = initPeer(host, port);

    bzero(msg, 100);

    printf("[PEER] Username: ");
    fflush(stdout);

    read(0, msg, 100);
    
    if(write(sd, msg, 100) <= 0) {
        perror("[PEER] Error: write() to server\n");
        exit(EXIT_FAILURE);
    }

    if(read(sd, msg, 100) < 0) {
        perror("[PEER] Error: read() from server\n");
        exit(EXIT_FAILURE);
    }

    printf ("[PEER] Received message: %s\n", msg);

    close(sd);
}
