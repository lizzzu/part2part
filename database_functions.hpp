#ifndef DB_FUNC_H
#define DB_FUNC_H

#include <sqlite3.h>

sqlite3* createDB();
void addUser();
void addFileToUser();
void getFileFromUser();
void removeUser();

#endif // DB_FUNC_H