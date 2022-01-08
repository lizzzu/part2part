#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>

#include "validation.hpp"

extern int errno;


typedef struct Users {
	int idUser = -1;
	char IPaddr[20];
	int port;
	char path[1000];
} Users;


int up_files = 0;
int listening_port = -1;
int sdServer = 0;


// peer doing stuff
int initPeer(const char* host, int port);                                // connect peer to central server
void runPeerServer(const char* filepath, const char* host, int port);    // when the peer uploads
int connectToPeer(const char* host, int port);                           // connect peer to other peer (for downloading)

void getPeerInput();
void disconnectPeer();

// file operations
void searchFile(const char* filename);
void downloadFile(const char* filepath, const char* host, int port);
void uploadFile(const char* path, const char* host, int port);


int main(int argc, char* argv[]) {
    printf("******** Welcome dear peer! ********\n");
    
    char host[20];
	int port;
    getIPandPort(host, port);

    sdServer = initPeer(host, port);
    while(1) {
        getPeerInput();
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

void runPeerServer(const char* filepath, const char* host, int port) {
	struct sockaddr_in peerServer;
    struct sockaddr_in from;

    int sdPeer;

    if((sdPeer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("[PEER_SERVER] Error: socket()");
        exit(EXIT_FAILURE);
    }

    int on = 1;
	setsockopt(sdPeer, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    bzero(&peerServer, sizeof(peerServer));
	bzero(&from, sizeof(from));

    peerServer.sin_family = AF_INET;
    peerServer.sin_addr.s_addr = inet_addr(host);
    peerServer.sin_port = htons(port);

    if(bind(sdPeer, (struct sockaddr*)&peerServer, sizeof(peerServer)) == -1) {
        perror("[PEER_SERVER] Error: bind()");
        exit(EXIT_FAILURE);
    }

	if(listen(sdPeer, CONNECTIONS) == -1) {
		perror("[PEER_SERVER] Error: listen()");
		exit(EXIT_FAILURE);
	}

    printf("Listening on port %d...\n", port);
    fflush(stdout);

    // blocant
    while(1) {
        int peer;
        unsigned int length = sizeof(from);
        if((peer = accept(sdPeer, (struct sockaddr*)&from, &length)) == -1) {
            perror("[PEER_SERVER] Error: accept()\n");
        }
        
        // sending the requested file
        printf("Found a downloader, transferring the file...\n");

        int fd;
        if((fd = open(filepath, O_RDONLY)) == -1) {
            close(sdPeer);
            perror("[PEER_SERVER] Error: open() filepath");
            exit(EXIT_FAILURE);
        }

        int nrbytes;
        char buff[SIZE];

        while(1) {
            if((nrbytes = read(fd, buff, SIZE)) == -1) {
                close(fd);
                perror("[PEER_SERVER] Error: read() from fd");
                exit(EXIT_FAILURE);
            }

            if(nrbytes == 0) { 
                close(peer);
                break;
            }

            if((write(peer, buff, nrbytes)) == -1) {
                close(peer);
                perror("[PEER_SERVER] Error: write() to peer");
                exit(EXIT_FAILURE);
            }
        }

        printf("Successfully transferred the file\n");

        printf("Do you wish to keep the file up? [y/n]\n");
        char option[2];
        scanf("%s", option);

        if(option[0] == 'n' || option[0] == 'N') {
            char send_msg[120];
            sprintf(send_msg, "nupload*%s", filepath);

            if((write(sdServer, send_msg, sizeof(send_msg))) == -1) {
                perror("[PEER_SERVER] Error: write() to peer");
                exit(EXIT_FAILURE);
            }

            close(sdPeer);

            break;
        }
        else if(option[0] == 'y' || option[0] == 'Y') {
            printf("OK, listening on port %d...\n", port);
        }
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

    if(connect(sdPeer, (struct sockaddr*)&peer, sizeof(peer)) == -1) {
        close(sdPeer);
        perror("[PEER] Error: connect() to other peer");
        return errno;
    }

    return sdPeer;
}

void getPeerInput() {
    printf("\n1 - Search file\n2 - Upload file\n3 - Exit\n> ");

    char opt[2];
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

        searchFile(filename);
    }
    else if(option == 2) {
        char IPaddr[20];
        strcpy(IPaddr, "0");
        
        int port;
        if(listening_port == -1) {
            printf("Enter your port number: ");

            char p[20];
            scanf("%s", p);
            while(!validatePort(p)) {
                printf("Invalid port number. Try again: ");
                scanf("%s", p);
            }

            port = atoi(p);
            listening_port = port;
        }
        else
            port = listening_port;

        char path[100];
        printf("Enter the file path you want to share: ");
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
        
        uploadFile(path, IPaddr, port);
    }
    else disconnectPeer();
}

void searchFile(const char* filename) {
    char msg[1200];
    sprintf(msg, "search*%s", filename);
    int msgsize = sizeof(msg);

    if(write(sdServer, msg, msgsize) <= 0) {
        perror("[PEER] Error: write() to server");
        exit(EXIT_FAILURE);
    }

    int countFiles = 0;
    if(read(sdServer, &countFiles, sizeof(int)) < 0) {
        perror("[PEER] Error: read() from server");
        exit(EXIT_FAILURE);
    }

    Users* usr = (struct Users*)malloc(sizeof(struct Users) * CONNECTIONS);
    int nrUsers = 0;

    printf("Found %d file(s)\n", countFiles);
    printf("------------------------------------\n\n");

    for(int i = 1; i <= countFiles; i++) {
        if(write(sdServer, &i, sizeof(int)) <= 0) {
            perror("[PEER] Error: write() to server");
            exit(EXIT_FAILURE);
        }
        
        char recv_msg[1200];
        if(read(sdServer, recv_msg, sizeof(recv_msg)) < 0) {
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

        printf("[File %d]\nPeer ID: %d\nHost: %s\nPort: %d\nShared file path: %s\n\n", 
            i, usr[i].idUser, usr[i].IPaddr, usr[i].port, usr[i].path);
        printf("------------------------------------\n\n");
    }

    if(countFiles > 1) {
        printf("Do you want to download any file? [y/n]\n");
        char option[2];
        scanf("%s", option);

        if(option[0] == 'y' || option[0] == 'Y') {
            printf("Which one? [1-%d]\n", countFiles);
            int file_number;
            scanf("%d", &file_number);

            downloadFile(usr[file_number].path, usr[file_number].IPaddr, usr[file_number].port);
        }
        else
            printf("As you wish\n");
    }
    else if(countFiles == 1) {
        printf("Do you want to download the file? [y/n]\n");
        char option[2];
        scanf("%s", option);

        if(option[0] == 'y' || option[0] == 'Y')
            downloadFile(usr[1].path, usr[1].IPaddr, usr[1].port);
        else
            printf("As you wish\n");
    }
    else
        printf("Sorry, there is no file called \"%s\"\n", filename);
}

void downloadFile(const char* filepath, const char* host, int port) {
    printf("Downloading %s from... ", filepath);
    printf("Host: %s | Port: %d\n", host, port);

    int sdPeer = connectToPeer(host, port);

    // downloading the file
    mkdir(DOWNLOAD_PATH, 0777);

    char path[120];
    char filename[100];
    strcpy(filename, strrchr(filepath, '/') + 1);
    sprintf(path, "%s/%s", DOWNLOAD_PATH, filename);

    int fd;
    if((fd = open(path, O_WRONLY | O_CREAT, 0666)) == -1) {
        close(sdPeer);
        perror("[PEER] Error: open() download path");
        exit(EXIT_FAILURE);
    }

    int nrbytes;
    char buff[SIZE];

    while(1) {
        if((nrbytes = read(sdPeer, buff, SIZE)) == -1) {
            close(sdPeer);
            perror("[PEER] Error: read() from peer");
            exit(EXIT_FAILURE);
        }

        if(nrbytes == 0) {
            close(fd);
            close(sdPeer);

            break;
        }

        if(write(fd, buff, nrbytes) == -1) {
            close(fd);
            perror("[PEER] Error: write() to fd");
        }
    }

    printf("Successfully downloaded the file \"%s\"\n", filename);
}

void uploadFile(const char* path, const char* host, int port) {
    char msg[1000];
    sprintf(msg, "upload*%s*%d*%s", host, port, path);
    int msgsize = sizeof(msg);

    Users* upload_list = (struct Users*)malloc(sizeof(struct Users) * CONNECTIONS);

    if(write(sdServer, msg, msgsize) <= 0) {
        perror("[PEER] Error: write() to server");
        exit(EXIT_FAILURE);
    }

    if(read(sdServer, msg, msgsize) < 0) {
        perror("[PEER] Error: read() from server");
        exit(EXIT_FAILURE);
    }

    up_files++;
    strcpy(upload_list[up_files].IPaddr, host);
    upload_list[up_files].port = port;
    strcpy(upload_list[up_files].path, path);

    char id[4];
    int i = 5;
    int idlg = 0;
    while(msg[i] != ']')
        id[idlg++] = msg[i++];
    id[idlg] = '\0';

    upload_list[up_files].idUser = atoi(id);

    char* recv_msg = strtok(msg, "\n");
    printf("%s\n", recv_msg);

    runPeerServer(path, host, port);
}

void disconnectPeer() {
    char msg[120] = "exit*";
    int msgsize = sizeof(msg);

    if(write(sdServer, msg, msgsize) <= 0) {
        perror("[PEER] Error: write() to server");
        exit(EXIT_FAILURE);
    }

    printf("Peer disconnected\n");
    exit(0);
}
