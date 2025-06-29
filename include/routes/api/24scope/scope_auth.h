#ifndef SCOPE_AUTH_H
#define SCOPE_AUTH_H
#include <microhttpd.h>
#include "post_iterator.h"

int handle_scope_login_verification(struct MHD_Connection *connection, const char *url, post_data_t *post_data);

#endif