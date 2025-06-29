
#include "db_connection.h"
#include <sqlite3.h>
#include <stdio.h>

sqlite3 *db = NULL; // Actual definition of the 'db' variable declared in db_connection.h

int db_init(const char *db_path) {
  if (sqlite3_open(db_path, &db) != SQLITE_OK) {
      fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
      return 0;
  }

  const char *create_table_sql =
      "CREATE TABLE IF NOT EXISTS users ("
      "id TEXT PRIMARY KEY,"
      "username TEXT NOT NULL,"
      "scope_token TEXT NOT NULL"
      ");";

  char *err_msg = NULL;
  if (sqlite3_exec(db, create_table_sql, 0, 0, &err_msg) != SQLITE_OK) {
      fprintf(stderr, "Failed to create table: %s\n", err_msg);
      sqlite3_free(err_msg);
      return 0;
  }

  return 1;
}

void db_close() {
  if (db) {
      sqlite3_close(db); // Close db connection
      db = NULL;
  }
}
