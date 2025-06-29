#include <microhttpd.h>
#include "post_iterator.h"
#include <stdio.h>

int get_post_body(
    post_data_t **out_post_data,
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *version,
    const char *upload_data,
    size_t *upload_data_size,
    void **con_cls)
{
  post_data_t *post_data = (post_data_t *) *con_cls;

  if (post_data == NULL)
  {
    post_data = calloc(1, sizeof(post_data_t));
    *con_cls = post_data;
    return MHD_YES;
  }

  if (*upload_data_size != 0)
  {
    size_t new_size = post_data->size + *upload_data_size;
    if (new_size + 1 > post_data->capacity)
    {
      size_t new_capacity = (new_size + 1) * 2;
      char *new_data = realloc(post_data->data, new_capacity);
      if (!new_data)
      {
        printf("Failed realloc new_data\n");
        return MHD_NO;
      }
      post_data->data = new_data;
      post_data->capacity = new_capacity;
    }
    memcpy(post_data->data + post_data->size, upload_data, *upload_data_size);
    post_data->size += *upload_data_size;
    post_data->data[post_data->size] = '\0';

    *upload_data_size = 0;
    return MHD_YES;
  }

  // Final call: process complete body (JSON is in post_data->data)
  printf("Received JSON:\n%s\n", post_data->data);
  
  // transfer ownership
  *out_post_data = post_data;
  *con_cls = NULL;

  return MHD_NO;
}