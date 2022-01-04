#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

void searchFile(int sd, const char* filename) {
    printf("[PEER] Filename: %s\n", filename);

    char msg[1200];
    sprintf(msg, "search*%s", filename);
    int msgsize = strlen(msg);

    if(write(sd, msg, msgsize) <= 0) {
        perror("[PEER] Error: write() to server");
        exit(EXIT_FAILURE);
    }

    int countFiles = 0;
    if(read(sd, &countFiles, sizeof(int)) < 0) {
        perror("[PEER] Error: read() from server");
        exit(EXIT_FAILURE);
    }

    printf("[PEER] Found %d file(s)\n", countFiles);
    printf("\n------------------------------------\n\n");

    for(int i = 1; i <= countFiles; i++) {
        if(write(sd, &i, sizeof(int)) <= 0) {
            perror("[PEER] Error: write() to server");
            exit(EXIT_FAILURE);
        }

        sleep(1);

        if(read(sd, msg, 1200) < 0) {
            perror("[PEER] Error: read() from server");
            exit(EXIT_FAILURE);
        }

        printf("%s", msg);
        printf("\n\n------------------------------------\n\n");
    }

}

void downloadFile() {
    printf("[[[downloadFile]]]\n");
}

void uploadFile(int sd, const char* path, const char* host, const char* port) {
    printf("Path: %s\n", path);

    char msg[1000];
    sprintf(msg, "upload*%s*%s*%s", host, port, path);
    int msgsize = strlen(msg);

    if(write(sd, msg, msgsize) <= 0) {
        perror("[PEER] Error: write() to server");
        exit(EXIT_FAILURE);
    }

    if(read(sd, msg, msgsize) < 0) {
        perror("[PEER] Error: read() from server");
        exit(EXIT_FAILURE);
    }

    msg[strlen(msg) - 3] = '\0';
    printf("[PEER] Received message: %s\n", msg);
}

#endif // FILE_OPERATIONS_H