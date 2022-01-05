#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "validation.hpp"
// #include "file_operations.hpp"

#define CONNECTIONS 20
#define SIZE 4096

extern int errno;

typedef struct Users {
	int idUser = -1;
	char IPaddr[20];
	int port;
	char path[1000];
} Users;

int up_files = 0;

// peer doing stuff
int initPeer(const char* host, int port);           // connect peer to central server
int createPeerServer(const char* host, int port);
void runPeerServer(const char* host, int port);     // when the peer uploads
int connectToPeer(const char* host, int port);      // connect peer to other peer (for downloading)

void getPeerInput(int sd);
void disconnectPeer(int sd);

// file operations
void searchFile(int sd, const char* filename);
void downloadFile(const char* host, int port, const char* filepath);
void uploadFile(int sd, const char* path, const char* host, const char* port);

int main(int argc, char* argv[]) {
    printf("******** Welcome dear peer! ********\n");
    
    char host[20];
	int port;
    getIPandPort(host, port);

    int sd = initPeer(host, port);
    while(1) {
        getPeerInput(sd);
    }
        
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

    if(connect(sdServer, (struct sockaddr*)&server, sizeof(server)) == -1) {
        close(sdServer);
        perror("[PEER] Error: connect()");
        return errno;
    }

    return sdServer;
}

int createPeerServer(const char* host, int port) {
    // PEER
    struct sockaddr_in peerServer;
    int sdPeer;

    if((sdPeer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[PEER] Error: socket()");
        return errno;
    }

    int on = 1;
	setsockopt(sdPeer, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&peerServer, sizeof(peerServer));

    printf("[PEER] sdPeer = %d\n", sdPeer);

    peerServer.sin_family = AF_INET;
    peerServer.sin_addr.s_addr = inet_addr(host);
    peerServer.sin_port = htons(port);

    if(bind(sdPeer, (struct sockaddr*)&peerServer, sizeof(peerServer)) == -1) {
        perror("[PEER] Error: bind()");
        return errno;
    }

    return sdPeer;
}

void runPeerServer(const char* host, int port) {
    struct sockaddr_in from;
	bzero(&from, sizeof(from));

	int sdPeer = createPeerServer(host, port);

	if(sdPeer == errno) {
		printf("[SERVER] Cannot create server\n");
		exit(1);
	}

	if(listen(sdPeer, CONNECTIONS) == -1) {
		perror("[SERVER] Error: listen()");
		exit(EXIT_FAILURE);
	}

    
}

int connectToPeer(const char* host, int port) {
    struct sockaddr_in peer;
    int sdPeer;

    if((sdPeer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[PEER] Error: socket()");
		return errno;
    }

    peer.sin_family = AF_INET;
    peer.sin_addr.s_addr = inet_addr(host);
    peer.sin_port = htons(port);

    if(connect(sdPeer, (struct sockaddr*)&peer, sizeof(peer))) {
        close(sdPeer);
        perror("[PEER] Error: connect()");
        return errno;
    }

    return sdPeer;
}

void getPeerInput(int sd) {
    char* opt;

    printf("\n1 - Search file\n2 - Upload files\n3 - Exit\n> ");
    scanf("%s", opt);
    int option = atoi(opt);

    while(option < 1 || option > 3 || !isdigit(opt[0]) || strlen(opt) > 1) {
        printf("Invalid option. Try again:\n> ");
        scanf("%s", opt);
        option = atoi(opt);
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
        
        uploadFile(sd, path, IPaddr, port);
    }
    else disconnectPeer(sd);
}

void searchFile(int sd, const char* filename) {
    printf("[PEER] Filename: %s\n", filename);

    char msg[1200];
    sprintf(msg, "search*%s", filename);
    int msgsize = sizeof(msg);

    if(write(sd, msg, msgsize) <= 0) {
        perror("[PEER] Error: write() to server");
        exit(EXIT_FAILURE);
    }

    int countFiles = 0;
    if(read(sd, &countFiles, sizeof(int)) < 0) {
        perror("[PEER] Error: read() from server");
        exit(EXIT_FAILURE);
    }

    Users* usr = (struct Users*)malloc(sizeof(struct Users) * CONNECTIONS);
    int nrUsers = 0;

    printf("[PEER] Found %d file(s)\n", countFiles);
    printf("------------------------------------\n");

    for(int i = 1; i <= countFiles; i++) {
        if(write(sd, &i, sizeof(int)) <= 0) {
            perror("[PEER] Error: write() to server");
            exit(EXIT_FAILURE);
        }
        
        char recv_msg[1200];
        if(read(sd, recv_msg, sizeof(recv_msg)) < 0) {
            perror("[PEER] Error: read() from server");
            exit(EXIT_FAILURE);
        }

        char* r = strtok(recv_msg, "*");
        usr[i].idUser = atoi(r);

        r = strtok(NULL, "*");
        strcpy(usr[i].IPaddr, r);
        if(usr[i].IPaddr[0] == '0' && strlen(usr[i].IPaddr) == 1)
            strcpy(usr[i].IPaddr, LOCALHOST);

        r = strtok(NULL, "*");
        usr[i].port = atoi(r);

        r = strtok(NULL, "*");
        strcpy(usr[i].path, r);

        printf("[File %d]\nPeer ID: %d\nIP address: %s\nPort: %d\nShared file path: %s\n", 
            i, usr[i].idUser, usr[i].IPaddr, usr[i].port, usr[i].path);
        printf("------------------------------------\n");
    }

    if(countFiles > 0) {
        printf("Which file do you want to donwlnoad? Enter a number between 1 and %d:\n", countFiles);
        int file_number;
        scanf("%d", &file_number);

        download(usr[file_number].IPaddr, usr[file_number].port, usr[file_number].path);
    }
    else
        ptinf("Sorry, there is no file called \"%s\"\n", filename);
}

void downloadFile(const char* host, int port, const char* filepath) {
    printf("Downloading from...\n");
    ptinf("%s | %d | %s\n", host, port, filepath);

    int sd = connectToPeer(host, port);

}

void uploadFile(int sd, const char* path, const char* host, int port) {
    char msg[1000];
    sprintf(msg, "upload*%s*%d*%s", host, port, path);
    int msgsize = sizeof(msg);

    Users* upload_list = (struct Users*)malloc(sizeof(struct Users) * CONNECTIONS);

    if(write(sd, msg, msgsize) <= 0) {
        perror("[PEER] Error: write() to server");
        exit(EXIT_FAILURE);
    }

    if(read(sd, msg, msgsize) < 0) {
        perror("[PEER] Error: read() from server");
        exit(EXIT_FAILURE);
    }

    up_files++;
    strcpy(upload_list[up_files].IPaddr, host);
    upload_list[up_files].port = atoi(port);
    strcpy(upload_list[up_files].path, path);

    char id[4];
    int i = 5;
    int idlg = 0;
    while(msg[i] != ']')
        id[idlg++] = msg[i++];
    id[idlg] = '\0';

    upload_list[up_files].idUser = atoi(id);

    char* recv_msg = strtok(msg, "\n");
    printf("[PEER] %s\n", recv_msg);

    runPeerServer(host, port);
}

void disconnectPeer(int sd) {
    char msg[120] = "exit*";
    int msgsize = sizeof(msg);

    if(write(sd, msg, msgsize) <= 0) {
        perror("[PEER] Error: write() to server");
        exit(EXIT_FAILURE);
    }

    printf("[PEER] Peer disconnected\n");
    exit(1);
}
