#include <curl/curl.h>
#include <microhttpd.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rpc.h>
#include <user_db.h>
#include <sign_cookie.h>
#include <time.h>
#include "post_iterator.h"
#include "env_reader.h"

struct string_buffer
{
  char *data;
  size_t size;
};

// Write callback to grow a string buffer dynamically
size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t total_size = size * nmemb;
  struct string_buffer *buf = (struct string_buffer *)userp;

  char *new_data = realloc(buf->data, buf->size + total_size + 1);
  if (!new_data)
    return 0;

  buf->data = new_data;
  memcpy(&(buf->data[buf->size]), contents, total_size);
  buf->size += total_size;
  buf->data[buf->size] = '\0';

  return total_size;
}

// Fourth and last step of OAuth2 flow, get user data and register in scope db
int get_discord_data(struct MHD_Connection *connection, const char *access_token)
{
  CURL *curl = curl_easy_init();
  if (!curl)
    return MHD_NO;

  struct string_buffer response = {0};
  response.data = malloc(1);
  response.size = 0;

  curl_easy_setopt(curl, CURLOPT_URL, "https://discord.com/api/v10/users/@me");

  char auth_header[512];
  snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", access_token);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, auth_header);
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

    free(response.data);
    curl_easy_cleanup(curl);

    return MHD_NO;
  }

  printf("Discord user response: %s\n", response.data);

  cJSON *json_root = cJSON_Parse(response.data);
  if (!json_root)
  {
    fprintf(stderr, "Failed to parse JSON.\n");

    free(response.data);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return MHD_NO;
  }

  cJSON *discord_id = cJSON_GetObjectItem(json_root, "id");
  cJSON *discord_username = cJSON_GetObjectItem(json_root, "username");
  if (!cJSON_IsString(discord_id) || !cJSON_IsString(discord_username))
  {
    fprintf(stderr, "No access_token in response.\n");
    cJSON_Delete(json_root);

    free(response.data);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    return MHD_NO;
  }

  // add user to db
  char *scope_token = NULL;
  if (db_insert_user(discord_id->valuestring, discord_username->valuestring, &scope_token))
  {
    // user created
    printf("scope-token of new user: %s\n", scope_token);

    char *signature = sign_scope_token(scope_token, env_get("COOKIE_SECRET"));

    // set scope-token cookie redirect user back to the scope
    struct MHD_Response *redirect_response = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(redirect_response, "Location", "/24scope");

    int days_valid = 30;
    time_t raw_time = time(NULL);
    raw_time += 60 * 60 * 24 * 30;

    struct tm *gmt_time = gmtime(&raw_time);

    char expires[64];
    strftime(expires, sizeof(expires), "%a, %d %b %Y %H:%M:%S GMT", gmt_time);

    char cookie_header[512];
    snprintf(cookie_header, sizeof(cookie_header), "scope-token=%s.%s; Expires=%s; SameSite=Lax; Path=/", scope_token, signature, expires);
    MHD_add_response_header(redirect_response, "Set-Cookie", cookie_header);

    int ret = MHD_queue_response(connection, MHD_HTTP_MOVED_PERMANENTLY, redirect_response);
    MHD_destroy_response(redirect_response);

    // Cleanup
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(response.data);
    free(scope_token);
  }
  else
  {
    // failed to create user
    const char *err = "500 Internal Server Error";
    struct MHD_Response *res = MHD_create_response_from_buffer(strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, res);
    MHD_destroy_response(res);

    printf("Failed to create user\n");

    // Cleanup
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    free(response.data);
    if (scope_token)
      free(scope_token);
    return ret;
  }
}

// Third step of OAuth2 flow
int exchange_code_for_token(struct MHD_Connection *connection, const char *code)
{
  CURL *curl = curl_easy_init();
  if (!curl)
    return MHD_NO;

  struct string_buffer response = {0};
  response.data = malloc(1);
  response.size = 0;

  char post_fields[1024];
  snprintf(post_fields, sizeof(post_fields),
           "client_id=%s&client_secret=%s&grant_type=authorization_code&code=%s&redirect_uri=%s",
           env_get("SCOPE_DC_CLIENT_ID"), env_get("SCOPE_DC_CLIENT_SECRET"), code, env_get("DC_OAUTH_REDIRECT_URI"));

  curl_easy_setopt(curl, CURLOPT_URL, "https://discord.com/api/v10/oauth2/token");
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK)
  {
    fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
    free(response.data);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return MHD_NO;
  }

  printf("Token exchange response: %s\n", response.data);

  cJSON *json_root = cJSON_Parse(response.data);
  if (!json_root)
  {
    fprintf(stderr, "Failed to parse JSON.\n");
    free(response.data);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return MHD_NO;
  }

  cJSON *token = cJSON_GetObjectItem(json_root, "access_token");
  if (!cJSON_IsString(token))
  {
    fprintf(stderr, "No access_token in response.\n");
    cJSON_Delete(json_root);
    free(response.data);
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
    return MHD_NO;
  }

  // Call the next step
  int result = get_discord_data(connection, token->valuestring);

  cJSON_Delete(json_root);
  free(response.data);
  curl_easy_cleanup(curl);
  curl_slist_free_all(headers);

  return result;
}

// Second step of OAuth2 flow
int handle_discord_oauth_redirect(struct MHD_Connection *connection, const char *url, post_data_t *post_data)
{
  (void)post_data;
  const char *code = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "code");
  if (!code)
  {
    const char *msg = "Missing authorization code";
    struct MHD_Response *res = MHD_create_response_from_buffer(strlen(msg), (void *)msg, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_BAD_REQUEST, res);
    MHD_destroy_response(res);
    return ret;
  }

  return exchange_code_for_token(connection, code);
}

// First step of OAuth2 flow
int handle_scope_login(struct MHD_Connection *connection, const char *url, post_data_t *post_data)
{
  (void)post_data;
  char discord_redirect_url[MAX_PATH];
  snprintf(discord_redirect_url, sizeof(discord_redirect_url),
           "https://discord.com/oauth2/authorize?response_type=code&client_id=%s&scope=%s&redirect_uri=%s",
           env_get("SCOPE_DC_CLIENT_ID"),
           env_get("DC_SCOPE"),
           env_get("DC_OAUTH_REDIRECT_URI"));

  struct MHD_Response *res = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
  MHD_add_response_header(res, "Location", discord_redirect_url);
  int ret = MHD_queue_response(connection, MHD_HTTP_FOUND, res);
  MHD_destroy_response(res);
  return ret;
}
