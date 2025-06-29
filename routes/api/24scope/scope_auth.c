#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <user_db.h>
#include <stdio.h>
#include <sign_cookie.h>
#include "post_iterator.h"

int handle_scope_login_verification(struct MHD_Connection *connection, const char *url, post_data_t *post_data)
{
  (void) post_data;
  const char *cookie_string = MHD_lookup_connection_value(connection, MHD_COOKIE_KIND, "scope-token");
  if(cookie_string == NULL) {
    printf("Missing scope-token cookie\n");
    const char* bad_request_string = "400 Bad Request";
    struct MHD_Response *response;
    int ret;
    response = MHD_create_response_from_buffer(strlen(bad_request_string), (void*)bad_request_string, MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
    MHD_destroy_response(response);

    return ret;
  }

  // cookie found
  printf("Raw scope-token cookie value: %s\n", cookie_string);
  char* scope_token;
  if(get_signed_cookie_value(cookie_string, &scope_token)) {
    printf("Sent token: %s\n", scope_token);

    User* user = db_get_user_by_scope_token(scope_token);

    if(user != NULL) {
      printf("Username: %s\n", user->username);
      struct MHD_Response *response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_MUST_FREE);
      int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      MHD_destroy_response(response);
      return ret;
    } else {
      printf("User not found\n");
      const char* response_string = "404 Not Found";
      struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_string), (void*)response_string, MHD_RESPMEM_MUST_FREE);
      int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      MHD_destroy_response(response);
      return ret;
    }
  }
}