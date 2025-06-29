#include "static_handler.h"
#include "post_iterator.h"
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <sys/stat.h>

#define STATIC_DIR "public"

const char *get_content_type(const char *url)
{
  if (strstr(url, ".html")) return "text/html";
  if (strstr(url, ".css")) return "text/css";
  if (strstr(url, ".js")) return "application/javascript";
  if (strstr(url, ".png")) return "image/png";
  if (strstr(url, ".jpg") || strstr(url, ".jpeg")) return "image/jpeg";
  if (strstr(url, ".gif")) return "image/gif";
  return "application/octet-stream";
}

int static_handler(struct MHD_Connection *connection, const char *url, post_data_t *post_data)
{
  (void) post_data;
  char file_path[1024];

  // Prevent path traversal
  if (strstr(url, "..") != NULL) {
    const char *err = "403 Forbidden";
    struct MHD_Response *res = MHD_create_response_from_buffer(strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_FORBIDDEN, res);
    MHD_destroy_response(res);
    return ret;
  }

  // Construct full file path
  snprintf(file_path, sizeof(file_path), "%s%s", STATIC_DIR, url);

  // If it's a directory but doesn't end in "/", redirect to slash version
  struct _stat st;
  if (_stat(file_path, &st) == 0 && (st.st_mode & _S_IFDIR)) {
    if (url[strlen(url) - 1] != '/') {
      // Redirect to slash version
      char redirect_url[1024];
      snprintf(redirect_url, sizeof(redirect_url), "%s/", url);
      struct MHD_Response *res = MHD_create_response_from_buffer(0, NULL, MHD_RESPMEM_PERSISTENT);
      MHD_add_response_header(res, "Location", redirect_url);
      int ret = MHD_queue_response(connection, MHD_HTTP_FOUND, res);
      MHD_destroy_response(res);
      return ret;
    }

    // Add index.html
    strncat(file_path, "/index.html", sizeof(file_path) - strlen(file_path) - 1);
  }

  printf("Trying to open: %s\n", file_path);
  FILE *fp = fopen(file_path, "rb");
  if (!fp) {
    const char *err = "404 Not Found";
    struct MHD_Response *res = MHD_create_response_from_buffer(strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, res);
    MHD_destroy_response(res);
    return ret;
  }

  // Get file size
  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  rewind(fp);

  char *buffer = malloc(file_size);
  if (!buffer) {
    fclose(fp);
    const char *err = "500 Internal Server Error";
    struct MHD_Response *res = MHD_create_response_from_buffer(strlen(err), (void *)err, MHD_RESPMEM_PERSISTENT);
    int ret = MHD_queue_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, res);
    MHD_destroy_response(res);
    return ret;
  }

  fread(buffer, 1, file_size, fp);
  fclose(fp);

  const char *content_type = get_content_type(file_path);
  struct MHD_Response *response = MHD_create_response_from_buffer(file_size, (void *)buffer, MHD_RESPMEM_MUST_FREE);
  MHD_add_response_header(response, "Content-Type", content_type);

  int ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
  MHD_destroy_response(response);
  return ret;
}
