#ifndef STATIC_HANDLER_H
#define STATIC_HANDLER_H

#include <microhttpd.h>
#include "post_iterator.h"

const char* get_content_type(const char*);
int static_handler(struct MHD_Connection *, const char *url, post_data_t *post_data);

#endif
