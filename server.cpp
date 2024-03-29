// skeleton: https://profs.info.uaic.ro/~computernetworks/files/NetEx/S12/ServerConcThread/servTcpConcTh2.c
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
#include "database_functions.hpp"

// extern int errno;

typedef struct thData {
	int idThread;
	int cl;
} thData;

typedef struct Users {
	int idUser = -1;
	char host[255];
	int port;
	int nrFiles = 0;
	char file[20][255];
	char path[20][255];
} Users;


Users* usr = (struct Users*)malloc(sizeof(struct Users) * CONNECTIONS);
int nrUsers = 0;

sqlite3* db;


void runServer(const char* host, int port);

static void* treat(void *);
void answerRequest(void *);


int main(int argc, char* argv[]) {
	char host[20];
	int port;
	getIPandPort(host, port);

	db = createDB();
	runServer(host, port);

    return 0;
}

void runServer(const char* host, int port) {
	struct sockaddr_in server;
	struct sockaddr_in from;

	int sd;

	if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("[SERVER] Error: socket()");
		exit(EXIT_FAILURE);
	}

	int on = 1;
	setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	bzero(&server, sizeof(server));
	bzero(&from, sizeof(from));

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(host);
	server.sin_port = htons(port);

	if(bind(sd, (struct sockaddr*) &server, sizeof(server)) == -1) {
		perror("[SERVER] Error: bind()");
		exit(EXIT_FAILURE);
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
	int cl = ++nrUsers;
	usr[cl].idUser = cl;
	
	bool run = true;

	do {
		char msg[1200] = "";

		if(read(tdL.cl, msg, 1000) <= 0) {
			printf("[Thread %d]\n", tdL.idThread);
			perror("Error: read() from client\n");	
		}
		
		printf("[Thread %d] The message has been received: %s\n", tdL.idThread, msg);

		char* p = strtok(msg, "*");

		switch(p[0]) {
		case 's': {
			p = strtok(NULL, " ");
			printf("[Thread %d] The client wants to search for: %s\n", tdL.idThread, p);

			getFileFromPeer(db, p);

			int foundFiles[20][20];
			int countFiles = 0;

			Users* foundUsers = (struct Users*)malloc(sizeof(struct Users) * CONNECTIONS);

			for(int i = 1; i <= nrUsers; i++) {
				for(int j = 1; j <= usr[i].nrFiles; j++) {
					if(strstr(usr[i].file[j], p) && usr[i].idUser != cl) {
						countFiles++;

						foundUsers[countFiles].idUser = i;
						foundUsers[countFiles].nrFiles = 1;

						foundUsers[countFiles].port = usr[i].port;
						strcpy(foundUsers[countFiles].host, usr[i].host);

						strcpy(foundUsers[countFiles].file[0], usr[i].file[j]);
						strcpy(foundUsers[countFiles].path[0], usr[i].path[j]);

						foundFiles[i][j] = 1;
					}
				}
			}

			if(write(tdL.cl, &countFiles, sizeof(int)) <= 0) {
				printf("[Thread %d] ", tdL.idThread);
				perror("[Thread] Error: write() to client");
			}
			else
				printf("[Thread %d] The number of files has been succesfully sent\n", tdL.idThread);

			for(int i = 1; i <= countFiles; i++) {
				char send_msg[1200];
				sprintf(send_msg, "%d*%s*%d*%s*", foundUsers[i].idUser, foundUsers[i].host, foundUsers[i].port, foundUsers[i].path[0]);
				int msgsize = sizeof(send_msg);

				int currentFile;
				if(read(tdL.cl, &currentFile, sizeof(int)) <= 0) {
					printf("[Thread %d]\n", tdL.idThread);
					perror("Error: read() from client\n");	
				}

				if(currentFile != i) {
					printf("currentFile = %d | i = %d\n", currentFile, i);
				}
				
				if(write(tdL.cl, send_msg, msgsize) <= 0) {
					printf("[Thread %d] ", tdL.idThread);
					perror("[Thread] Error: write() to client\n");
				}
				else
					printf("[Thread %d] The message %s has been succesfully sent\n", tdL.idThread, send_msg);
			}

			break;
		}
		case 'u': {
			p = strtok(NULL, "*");
			strcpy(usr[cl].host, p);
			if(usr[cl].host[0] == '0' && strlen(usr[cl].host) == 1)
				strcpy(usr[cl].host, LOCALHOST);

			p = strtok(NULL, "*");
			usr[cl].port = atoi(p);

			p = strtok(NULL, "\0");
			printf("\n[Thread %d] The client %d (Host: %s | Port: %d) wants to upload: %s\n", 
				tdL.idThread, usr[cl].idUser, usr[cl].host, usr[cl].port, p);

			usr[cl].nrFiles++;
			strcpy(usr[cl].path[usr[cl].nrFiles], p);
			strcpy(usr[cl].file[usr[cl].nrFiles], strrchr(p, '/') + 1);
			
			printf("User ID: %d\n", usr[cl].idUser);
			printf("Number of files: %d\n", usr[cl].nrFiles);
			for(int f = 1; f <= usr[cl].nrFiles; f++)
				printf("Shared file: %20s | Path: %s\n", usr[cl].file[f], usr[cl].path[f]);
			
			addPeer(db, usr[cl].idUser, usr[cl].host, usr[cl].port, usr[cl].file[usr[cl].nrFiles], usr[cl].path[usr[cl].nrFiles]);

			char send_msg[100];
			sprintf(send_msg, "[ID = %d] Uploading: %s\n", usr[cl].idUser, p);
			int msgsize = sizeof(send_msg);
			if(write(tdL.cl, send_msg, msgsize) <= 0) {
				printf("[Thread %d] ", tdL.idThread);
				perror("[Thread] Error: write() to client");
			}
			else
				printf("[Thread %d] The message %s has been succesfully sent\n", tdL.idThread, send_msg);

			break;
		}
		case 'n' : {
			printf("[Thread %d] The client %d stopped uploading\n", tdL.idThread, usr[cl].idUser);
			
			removePeer(db, usr[cl].idUser);
			usr[cl].nrFiles--;

			break;
		}
		case 'e': {
			printf("[Thread %d] The client %d disconnected from the server\n", tdL.idThread, usr[cl].idUser);

			removePeer(db, usr[cl].idUser);
			usr[cl].idUser = -1;
			usr[cl].nrFiles = 0;

			// nrUsers--;

			run = false;
			break;
		}
		default:
			break;
		}

		printf("\n");

	} while(run);
}
