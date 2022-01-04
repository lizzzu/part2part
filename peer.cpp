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

int initPeer(const char* host, int port);       // connect peer to central server
int createPeerServer();                         // duuh
int connectToPeer(const char* host, int port);  // connect peer to other peer

// peer doing stuff
void getPeerInput(int sd);
void sendRequest(const char* host, int port);
void disconnectPeer(int sd);

int main(int argc, char* argv[]) {
    printf("******** Welcome dear peer! ********\n");
    
    char host[20];
	int port;
    getIPandPort(host, port);

    int sd = initPeer(host, port);
    while(1) {
        getPeerInput(sd);
    }
    
    // sendRequest(host, port);
    
    return 0;
}

int initPeer(const char* host, int port) {
    struct sockaddr_in server;
    int sdServer;

    if((sdServer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[PEER] Error: socket()");
		return errno;
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(host);
    server.sin_port = htons(port);

    if(connect(sdServer, (struct sockaddr*) &server, sizeof(server)) == -1) {
        close(sdServer);
        perror("[PEER] Error: connect()");
        return errno;
    }

    return sdServer;
}

// int createPeerServer() {
//     // IP address and port number
//     char host[20];
//     int port;

//     getIPandPort(host, port);

//     // PEER
//     struct sockaddr_in peer;
//     int sdPeer;

//     if((sdPeer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//         perror("[PEER] Error: socket()");
//         return errno;
//     }

//     int on = 1;
// 	   setsockopt(sdPeer, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

//     bzero(&peer, sizeof(peer));

//     printf("[PEER] sdPeer = %d\n", sdPeer);

//     peer.sin_family = AF_INET;
//     peer.sin_addr.s_addr = inet_addr(LOCALHOST);
//     peer.sin_port = htons(port);

//     if(bind(sdPeer, (struct sockaddr*) &peer, sizeof(peer)) == -1) {
//         perror("[PEER] Error: bind()");
//         return errno;
//     }

//     return sdPeer;
// }

// int connectToPeer(const char* host, int port) {
//     struct sockaddr_in peer;
//     int sdPeer;

//     if((sdPeer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
//         perror("[PEER] Error: socket()");
// 		return errno;
//     }

//     printf("[PEER] sdPeer = %d\n", sdPeer);

//     peer.sin_family = AF_INET;
//     peer.sin_addr.s_addr = inet_addr(host);
//     peer.sin_port = htons(port);

//     if(connect(sdPeer, (struct sockaddr*) &peer, sizeof(peer))) {
//         // close(sdPeer);
//         perror("[PEER] Error: connect()");
//         return errno;
//     }

//     return sdPeer;
// }

void getPeerInput(int sd) {
    int option;

    printf("\n1 - Search file\n2 - Upload files\n3 - Exit\n> ");
    scanf("%d", &option);

    while(option < 1 || option > 3) {
        printf("Invalid option. Try again:\n> ");
        scanf("%d", &option);
    }

    if(option == 1) {
        char filename[100];
        
        printf("Search the file you want to download: ");
        do {
            fflush(stdout);
            read(0, filename, sizeof(filename));
        } while(filename[0] == '\n');

        for(int i = 0; i < strlen(filename); i++)
            if(filename[i] == '\n') {
                filename[i] = '\0';
                break;
            }

        searchFile(sd, filename);
    }
    else if(option == 2) {
        char IPaddr[20];
        char p[20];

        printf("Enter your IP address (enter 0 for localhost): ");
        scanf("%s", IPaddr);
        while(!validateIPaddr(IPaddr)) {
            printf("Invalid IP address. Try again: ");
            scanf("%s", IPaddr);
        }

        printf("Enter your port number: ");
        scanf("%s", p);
        while(!validatePort(p)) {
            printf("Invalid port number. Try again: ");
            scanf("%s", p);
        }
        int port = atoi(p);

        char path[100];
        printf("Enter the directory path you want to share: ");
        do {
            fflush(stdout);
            read(0, path, sizeof(path));
        } while(path[0] == '\n');

        for(int i = 0; i < strlen(path); i++)
            if(path[i] == '\n') {
                path[i] = '\0';
                break;
            }
        
        if(!validatePath(path)) {
            perror("Invalid path");
            exit(EXIT_FAILURE);
        }
        
        uploadFile(sd, path, IPaddr, p);
    }
    else disconnectPeer(sd);
}

// void sendRequest(const char* host, int port) {
//     char msg[100];

//     int sd = initPeer(host, port);

//     bzero(msg, 100);

//     printf("[PEER] Username: ");
//     fflush(stdout);

//     read(0, msg, 100);
    
//     if(write(sd, msg, 100) <= 0) {
//         perror("[PEER] Error: write() to server\n");
//         exit(EXIT_FAILURE);
//     }

//     if(read(sd, msg, 100) < 0) {
//         perror("[PEER] Error: read() from server\n");
//         exit(EXIT_FAILURE);
//     }

//     printf ("[PEER] Received message: %s\n", msg);

//     close(sd);
// }

void disconnectPeer(int sd) {
    char msg[120] = "exit*";
    int msgsize = strlen(msg);

    if(write(sd, msg, msgsize) <= 0) {
        perror("[PEER] Error: write() to server");
        exit(EXIT_FAILURE);
    }

    printf("[PEER] Peer disconnected");
    exit(1);
}
