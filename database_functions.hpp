#ifndef DB_FUNCTIONS_H
#define DB_FUNCTIONS_H

#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>

sqlite3* createDB() {
    sqlite3* db;
    char* err_msg = 0;

    if (sqlite3_open("peer_info.db", &db) != SQLITE_OK) {
        perror("Failed to open database");
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }

    char sql[1024];
    sprintf(sql, "DROP TABLE IF EXISTS peer_info;"
                 "CREATE TABLE peer_info (id INTEGER NOT NULL, host VARCHAR(255) NOT NULL, port INTEGER NOT NULL, file VARCHAR(255), path VARCHAR(255));");

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("Error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
    else {
        printf("Database successfully created\n\n");
    }

    return db;
}

void addPeer(sqlite3* db, int id, const char* host, int port, const char* file, const char* path) {
    char* err_msg = 0;

    char sql[2048];
    sprintf(sql, "INSERT INTO peer_info VALUES (%d, '%s', %d, '%s', '%s');", id, host, port, file, path);

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("Error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
    else {
        printf("Succesfully inserted into database\n");
    }
}

int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    NotUsed = 0;
    for (int i = 0; i < argc; i++)
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    printf("\n");
    return 0;
}

void getFileFromPeer(sqlite3* db, const char* filename) { // from https://zetcode.com/db/sqlitec/
    char* err_msg = 0;
    char sql[1024];
    sprintf(sql, "SELECT id, host, port, path FROM peer_info WHERE file LIKE '%%%s%%';", filename);

    int rc = sqlite3_exec(db, sql, callback, 0, &err_msg);
    if (rc != SQLITE_OK ) {
        perror("Failed to select data\n");
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        exit(EXIT_FAILURE);
    } 
}

void removePeer(sqlite3* db, int id) {
    char* err_msg = 0;

    char sql[2048];
    sprintf(sql, "DELETE FROM peer_info WHERE id = %d;", id);

    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK) {
        printf("Error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        exit(EXIT_FAILURE);
    }
    else {
        printf("Succesfully deleted from database\n");
    }
}

#endif // DB_FUNCTIONS_H
