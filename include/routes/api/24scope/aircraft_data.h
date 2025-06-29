#ifndef AIRCRAFT_DATA_H
#define AIRCRAFT_DATA_H

#include <microhttpd.h>
#include "post_iterator.h"

int handle_aircraft_data_get(struct MHD_Connection *connection, const char *url, post_data_t *post_data);

enum MHD_Result post_iterator(void *cls, enum MHD_ValueKind kind,
                              const char *key, const char *filename,
                              const char *content_type, const char *transfer_encoding,
                              const char *data, uint64_t off, size_t size);

int handle_aircraft_data_post(struct MHD_Connection *connection, const char *url, post_data_t *post_data);

#endif
