#ifndef DB_CONNECTION_H
#define DB_CONNECTION_H

#include <sqlite3.h>

// Declare the 'db' variable as external, meaning it is defined elsewhere
extern sqlite3 *db;

// Function declarations that interact with the database
int db_init(const char* db_path);
void db_close();

#endif