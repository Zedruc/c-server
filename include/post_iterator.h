#ifndef POST_ITERATOR_H
#define POST_ITERATOR_H

typedef struct
{
  char *data;
  size_t size;
  size_t capacity;
} post_data_t;

int get_post_body(
                    post_data_t **out_post_data,
                    struct MHD_Connection *connection,
                    const char *url,
                    const char *method,
                    const char *version,
                    const char *upload_data,
                    size_t *upload_data_size,
                    void **con_cls);

#endif /* POST_ITERATOR_H */