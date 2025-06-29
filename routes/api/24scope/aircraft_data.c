#include <microhttpd.h>
#include <stdlib.h>
#include <string.h>
#include <user_db.h>
#include <stdio.h>
#include <sign_cookie.h>
#include "post_iterator.h"
#include "env_reader.h"

//
char *latest_aicraft_data = NULL;
//

int handle_aircraft_data_get(struct MHD_Connection *connection, const char *url, post_data_t *post_data)
{
  (void) post_data;
  const char *cookie_string = MHD_lookup_connection_value(connection, MHD_COOKIE_KIND, "scope-token");
  if (cookie_string == NULL)
  {
    const char *bad_request_string = "400 Bad Request";
    struct MHD_Response *response;
    int ret;
    response = MHD_create_response_from_buffer(strlen(bad_request_string), (void *)bad_request_string, MHD_RESPMEM_PERSISTENT);
    ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, response);
    MHD_destroy_response(response);

    return ret;
  }

  // cookie found
  char *scope_token;
  if (get_signed_cookie_value(cookie_string, &scope_token))
  {
    User *user = db_get_user_by_scope_token(scope_token);

    if (user != NULL)
    {
      if(latest_aicraft_data == NULL)
      {
        const char *error_message = "{}";
        struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_message), (void *)error_message, MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Content-Type", "application/json");
        int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
        MHD_destroy_response(response);
        return ret;
      }
      struct MHD_Response *response = MHD_create_response_from_buffer(strlen(latest_aicraft_data), (void *)latest_aicraft_data, MHD_RESPMEM_PERSISTENT);
      MHD_add_response_header(response, "Content-Type", "application/json");
      int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      MHD_destroy_response(response);
      return ret;
    }
    else
    {
      const char *response_string = "404 Not Found";
      struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_string), (void *)response_string, MHD_RESPMEM_PERSISTENT);
      int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
      MHD_destroy_response(response);
      return ret;
    }
  }
  else
  {
    const char *response_string = "500 Internal Server Error";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_string), (void *)response_string, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
    MHD_destroy_response(response);
    return ret;
  }
}

static enum MHD_Result post_processor(void *cls,
                                      enum MHD_ValueKind kind,
                                      const char *key,
                                      const char *filename,
                                      const char *content_type,
                                      const char *transfer_encoding,
                                      const char *data,
                                      uint64_t off,
                                      size_t size)
{
  post_data_t *post_data = cls;
  printf("Received mime-type %s\n", content_type);

  // Reallocate to accumulate data
  char *new_data = realloc(post_data->data, post_data->size + size + 1);
  if (new_data == NULL)
  {
    printf("Failed to realloc in aircraft_data_post post_processor\n");
    return MHD_NO;
  }
  post_data->data = new_data;
  memcpy(post_data->data + post_data->size, data, size);
  post_data->size += size;
  post_data->data[post_data->size] = '\0'; // Null-terminate

  return MHD_YES;
}

int handle_aircraft_data_post(struct MHD_Connection *connection, const char *url, post_data_t *post_data)
{
  printf("POST received\n");
  const char *ptfs_auth_token = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "authorization");
  printf("Authorization Header: %s\n", ptfs_auth_token);
  if (ptfs_auth_token == NULL || strcmp(ptfs_auth_token, env_get("ATC24_AUTH_TOKEN")) != 0)
  {
    const char *response_string = "401 Unauthorized";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(response_string), (void *)response_string, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_UNAUTHORIZED, response);
    MHD_destroy_response(response);
    return ret;
  }

  size_t acdata_size = post_data->size;

  // Free old data if present
  if (latest_aicraft_data != NULL)
  {
    free(latest_aicraft_data);
  }

  latest_aicraft_data = strdup(post_data->data);
  if (latest_aicraft_data == NULL)
  {
    printf("Failed to strdup latest_aircraft_data\n");
    const char *error_response = "Failed to store aircraft data";
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(error_response), (void *)error_response, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, response);
    MHD_destroy_response(response);
    return ret;
  }

  printf("latest_aircraft_data:\n%s\n", latest_aicraft_data);

  // Example: you could parse post_data->data using cJSON
  // cJSON *json = cJSON_Parse(post_data->data);
  // do something...

  // Respond to sender
  const char *ok_response = "{\"status\": 200, \"message\": \"Aircraft data received\"}";
  struct MHD_Response *response = MHD_create_response_from_buffer(strlen(ok_response), (void *)ok_response, MHD_RESPMEM_PERSISTENT);
  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);

  return ret;
}
