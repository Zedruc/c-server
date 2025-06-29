#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc.h>
#include <db_connection.h>
#include <user_db.h>

char* generate_uuid_string() {
  RPC_CSTR uuid_rpc_cstr = NULL;
  UUID uuid;
  
  if (UuidCreate(&uuid) != RPC_S_OK || UuidToStringA(&uuid, &uuid_rpc_cstr) != RPC_S_OK) {
    return NULL;  // Or handle error appropriately
}
  
  char *uuid_copy = strdup((char*)uuid_rpc_cstr);
  RpcStringFreeA(&uuid_rpc_cstr);

  return uuid_copy;
}

int db_insert_user(const char *id, const char *username, char** out_uuid) {
    if(db == NULL) {
      fprintf(stderr, "DB not initialized\n");
      return 0;
    }
    const char *insert_sql = "INSERT OR REPLACE INTO users (id, username, scope_token) VALUES (?, ?, ?);";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, insert_sql, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }

    char *uuid = generate_uuid_string();
    if (!uuid) {
      fprintf(stderr, "Failed to generate UUID\n");
      return 0;
    }

    sqlite3_bind_text(stmt, 1, id, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, username, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, uuid, -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Insert failed: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        return 0;
    }

    // done with insert
    printf("[user_db] User %s inserted\n", uuid);
    sqlite3_finalize(stmt);
    
    // pass uuid to the calling functions variable
    *out_uuid = uuid;
    return 1;
}

User* db_get_user_by_discord_id(const char *discord_id) {
  if (db == NULL) {
      fprintf(stderr, "DB not initialized\n");
      return NULL;
  }

  const char *query_sql = "SELECT id, username, scope_token FROM users WHERE id = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query_sql, -1, &stmt, NULL) != SQLITE_OK) {
      fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
      return NULL;
  }

  sqlite3_bind_text(stmt, 1, discord_id, -1, SQLITE_TRANSIENT);

  User *user = NULL;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
      user = malloc(sizeof(User));
      user->id = strdup((const char *)sqlite3_column_text(stmt, 0));
      user->username = strdup((const char *)sqlite3_column_text(stmt, 1));
      user->scope_token = strdup((const char *)sqlite3_column_text(stmt, 2));
  }

  sqlite3_finalize(stmt);
  return user;
}

User* db_get_user_by_scope_token(const char *scope_token) {
  if (db == NULL) {
      fprintf(stderr, "DB not initialized\n");
      return NULL;
  }

  const char *query_sql = "SELECT id, username, scope_token FROM users WHERE scope_token = ?;";
  sqlite3_stmt *stmt;

  if (sqlite3_prepare_v2(db, query_sql, -1, &stmt, NULL) != SQLITE_OK) {
      fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
      return NULL;
  }

  sqlite3_bind_text(stmt, 1, scope_token, -1, SQLITE_TRANSIENT);

  User *user = NULL;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
      user = malloc(sizeof(User));
      user->id = strdup((const char *)sqlite3_column_text(stmt, 0));
      user->username = strdup((const char *)sqlite3_column_text(stmt, 1));
      user->scope_token = strdup((const char *)sqlite3_column_text(stmt, 2));
  }

  sqlite3_finalize(stmt);
  return user;
}

void db_free_user(User *user) {
  if (user) {
      free(user->id);
      free(user->username);
      free(user->scope_token);
      free(user);
  }
}