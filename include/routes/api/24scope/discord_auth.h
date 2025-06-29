#ifndef DISCORD_AUTH_H
#define DISCORD_AUTH_H

#include <microhttpd.h>
#include "post_iterator.h"

int handle_scope_login(struct MHD_Connection *connection, const char *url, post_data_t *post_data);

size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);

int get_discord_data(struct MHD_Connection *connection, const char* access_token);

int exchange_code_for_token(struct MHD_Connection *connection, const char *code);

int handle_discord_oauth_redirect(struct MHD_Connection *connection, const char *url, post_data_t *post_data);

#endif