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
#include <sqlite3.h>

#include "validation.hpp"
// #include "database_functions.hpp"

#define CONNECTIONS 20
#define THREADS 100

extern int errno;

typedef struct thData {
	int idThread;
	int cl;
} thData;

typedef struct Users {
	int idUser;
	char IPaddr[20];
	int port;
	int nrFiles = 0;
	char file[20][100];
	char path[20][1000];
} Users;
Users* usr = (struct Users*)malloc(sizeof(struct Users) * CONNECTIONS);

sqlite3* db;

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
	struct sockaddr_in from;
	bzero(&from, sizeof(from));

	int sd = createServer(host, port);

	if(sd == errno) {
		printf("[SERVER] Cannot create server\n");
		exit(1);
	}

	if(listen(sd, CONNECTIONS) == -1) {
		perror("[SERVER] Error: listen()");
		exit(EXIT_FAILURE);
	}

	pthread_t th[THREADS];
	thData* td = (struct thData*)malloc(sizeof(struct thData) * THREADS);
	unsigned int i = 0;
	
	while(1) {
		int client;
		unsigned int length = sizeof(from);

        printf("[SERVER] Waiting on port %d...\n", port);
        fflush(stdout);

        if((client = accept(sd, (struct sockaddr*)&from, &length)) == -1) {
            perror("[SERVER] Error: accept()\n");
            continue;
        }

        td[i].idThread = i;
        td[i].cl = client;

		pthread_create(&th[i], NULL, &treat, td + i);
		i++;
	}
}

static void* treat(void* arg) {
	struct thData tdL = *((struct thData*)arg);

	printf("[Thread %d] Waiting for the message...\n", tdL.idThread);
	fflush(stdout);
	pthread_detach(pthread_self());	
	answerRequest((struct thData*)arg);

    close((intptr_t)arg);
    return(NULL);
}

void answerRequest(void* arg) {
	struct thData tdL = *((struct thData*)arg);
	int cl = tdL.cl;
	
	bool run = true;

	do {
		char msg[100] = "";
		int i = 0;

		if(read(tdL.cl, msg, 100) <= 0) {
			printf("[Thread %d]\n", tdL.idThread);
			perror("Error: read() from client\n");	
		}
		
		printf("[Thread %d] The message has been received: %s\n", tdL.idThread, msg);

		char* p = strtok(msg, "*");

		switch(p[0]) {
		case 's':
			strcpy(msg, msg + strlen(p) + 1);
			printf("[Thread %d] The client wants to search for: %s\n", tdL.idThread, p);

			if(write(tdL.cl, msg, sizeof(msg)) <= 0) {
				printf("[Thread %d] ", tdL.idThread);
				perror("[Thread] Error: write() to client\n");
			}
			else
				printf("[Thread %d] The message has been succesfully sent\n", tdL.idThread);
			
			break;
		
		case 'd':
			strcpy(msg, msg + strlen(p) + 1);
			printf("[Thread %d] The client wants to download from: %s\n", tdL.idThread, p);

			break;

		case 'u':
			p = strtok(NULL, "*");
			strcpy(usr[cl].IPaddr, p);

			p = strtok(NULL, "*");
			usr[cl].port = atoi(p);

			p = strtok(NULL, "\0");
			strcpy(msg, p);
			
			printf("[Thread %d] The client %d (IP address: %s | Port: %d) wants to upload: %s\n", 
				tdL.idThread, usr[cl].idUser, usr[cl].IPaddr, usr[cl].port, msg);

			usr[cl].idUser = tdL.cl;
			strcpy(usr[cl].path[usr[cl].nrFiles], msg);
			strcpy(usr[cl].file[usr[cl].nrFiles], strrchr(msg, '/') + 1);

			printf("User ID: %d\n", usr[cl].idUser);
			printf("Nr of files: %d\n", usr[cl].nrFiles + 1);
			for(int f = 0; f <= usr[cl].nrFiles; f++)
				printf("Shared file: %20s | Path: %s\n", usr[cl].file[f], usr[cl].path[f]);

			usr[cl].nrFiles++;

			if(write(cl, msg, sizeof(msg)) <= 0) {
				printf("[Thread %d] ", tdL.idThread);
				perror("[Thread] Error: write() to client\n");
			}
			else
				printf("[Thread %d] The message has been succesfully sent\n", tdL.idThread);

			break;
		
		case 'e':
			printf("[Thread %d] The client disconnected from the server\n", tdL.idThread);

			run = false;
			break;
		
		default:
			break;
		}

	} while(run == true);
}
