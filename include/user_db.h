#ifndef USER_DB_H
#define USER_DB_H

char* generate_uuid_string();

int db_insert_user(const char *id, const char *username, char **out_uuid);

typedef struct {
  char *id;
  char *username;
  char *scope_token;
} User;

User* db_get_user_by_discord_id(const char *discord_id);

User* db_get_user_by_scope_token(const char *scope_token);

void db_free_user(User *user);

#endif