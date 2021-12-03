#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "validation.hpp"
#include "file_operations.hpp"

extern int errno;

int initPeer(const char* host, int port);
void getPeerInput();
void sendRequest(const char* host, int port);
void disconnectPeer();

int main(int argc, char* argv[]) {
    printf("******** Welcome dear peer! ********\n");

    char host[20];
	int port;
    
    getIPandPort(host, port);
    initPeer(host, port);

    getPeerInput();
    
    // sendRequest(host, port);
    
    return 0;
}

int initPeer(const char* host, int port) {
    struct sockaddr_in server;
    struct sockaddr_in peer;
    int sdServer;
    int sdPeer;

    // connect to SERVER
    if((sdServer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[PEER] Error: socket()");
		return errno;
    }

    printf("[PEER] sdServer = %d\n", sdPeer);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_port = htons(port);

    if(connect(sdServer, (struct sockaddr*) &server, sizeof(server))) {
        close(sdServer);
        perror("[PEER] Error: connect()\n");
        return errno;
    }

    // PEER
    if((sdPeer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[PEER] Error: socket()");
        return errno;
    }

    int on = 1;
	setsockopt(sdPeer, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    printf("[PEER] sdPeer = %d\n", sdPeer);

    peer.sin_family = AF_INET;
    peer.sin_addr.s_addr = htonl(INADDR_ANY);
    peer.sin_port = htons(port);

    if(bind(sdPeer, (struct sockaddr*) &peer, sizeof(peer)) == -1) {
        perror("[PEER] Error: bind()");
        return errno;
    }

    return sdPeer;
}

void getPeerInput() {
    int option;

    printf("[1] Search file\n[2] Upload files\n[3] Exit\n> ");
    scanf("%d", &option);

    while(option < 1 || option > 3) {
        printf("Invalid option. Try again:\n> ");
        scanf("%d", &option);
    }

    if(option == 1) searchFile();
    else if(option == 2) uploadFile();
    else disconnectPeer();
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

void disconnectPeer() {
    printf("Peer disconnected\n");
}