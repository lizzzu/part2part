#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "validation.hpp"

extern int errno;

int initPeer(const char* host, int port);
int getPeerInput();
void sendRequest(const char* host, int port);

void searchFile(const char* filename);
void downloadFile();
void uploadFile();

int main(int argc, char* argv[]) {
    printf("******** Welcome dear peer! ********\n");

    char host[20];
	uint16_t port;
    
    getIPandPort(host, port);
    initPeer(host, port);

    // getPeerInput();
    
    // sendRequest(host, port);
    
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

int getPeerInput() {
    return 0;
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
