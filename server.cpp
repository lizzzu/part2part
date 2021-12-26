#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "validation.hpp"

#define MAX_CONNECTIONS 20
#define MAX_THREADS 100

extern int errno;

typedef struct thData {
	int idThread;
	int cl;
} thData;

int createServer(const char* host, int port);
void runServer(const char* host, int port);

static void* treat(void *);
void answerRequest(void *);

int main(int argc, char* argv[]) {
	char host[20];
	int port;
	getIPandPort(host, port);

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
	pthread_t th[MAX_THREADS];

	struct sockaddr_in from;
	bzero(&from, sizeof(from));

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

	unsigned int i = 0;
	
	while(1) {
		int client;
        thData *td;
		unsigned int length = sizeof(from);

        printf("[SERVER] Waiting on port %d...\n", port);
        fflush(stdout);

        if((client = accept(sd, (struct sockaddr*) &from, &length)) == -1) {
            perror("[SERVER] Error: accept()\n");
            continue;
        }

        td = (struct thData*) malloc(sizeof(struct thData));	
        td->idThread = i++;
        td->cl = client;

        pthread_create(&th[i], NULL, &treat, td);
	}
}

static void* treat(void *arg) {
	struct thData tdL;
    tdL = *((struct thData*) arg);	
    
    printf("[Thread %d] Waiting for the message...\n", tdL.idThread);
    fflush(stdout);		 
    pthread_detach(pthread_self());		
    answerRequest((struct thData*) arg);

    close((intptr_t) arg);
    return(NULL);
}

void answerRequest(void *arg) {
	char msg[100];
	int i = 0;
	struct thData tdL; 
	tdL = *((struct thData*) arg);

	if(read(tdL.cl, msg, 100) <= 0) {
        printf("[Thread %d]\n", tdL.idThread);
		perror("Error: read() from client\n");	
	}
	
	printf("[Thread %d] The message has been received: %s\n", tdL.idThread, msg);

	char* p = strtok(msg, "*");
	if(strcmp(p, "search") == 0) {
		// p = strtok(NULL, "");
		strcpy(msg, msg + strlen("search") + 1);
		printf("[Thread %d] The client wants to search for: %s\n", tdL.idThread, p);
	}
	else if(strcmp(p, "download") == 0) {
		// p = strtok(NULL, " ");
		strcpy(msg, msg + strlen("download") + 1);
		printf("[Thread %d] The client wants to download from: %s\n", tdL.idThread, p);
	}
	else if(strcmp(p, "upload") == 0) {
		// p = strtok(NULL, " ");
		strcpy(msg, msg + strlen("upload") + 1);
		printf("[Thread %d] The client want to upload from: %s\n", tdL.idThread, p);
	}
	else if(strcmp(p, "exit") == 0) {
		strcpy(msg, "Peer disconnected");
		printf("[Thread %d] The client disconnected from the server\n", tdL.idThread);
	}

	if(write(tdL.cl, msg, sizeof(msg)) <= 0) {
		printf("[Thread %d] ", tdL.idThread);
		perror("[Thread] Error: write() to client\n");
	}
	else
		printf ("[Thread %d] The message has been succesfully sent\n", tdL.idThread);	
}
