#ifndef ROUTER_H
#define ROUTER_H

#include <microhttpd.h>
#include "post_iterator.h"

// A route handler function type
typedef int (*route_handler_t)(struct MHD_Connection *connection, const char *url, post_data_t *post_data);

// Define a struct to represent each route
typedef struct
{
  char *method;            // The HTTP Method (e.g., GET, POST, DELETE, etc.)
  char *path;              // The route path (e.g., "/api/users/{id}")
  route_handler_t handler; // The handler for that path
} route_t;

// Router struct that holds the list of routes
typedef struct
{
  route_t *routes;    // Array of routes
  size_t route_count; // Number of routes
  char *static_path;
  route_handler_t static_content_handler; // Handle static content e.g. websites
} router_t;

// Initialize the router
router_t *router_init();

// Register a new route
void router_register(router_t *router, const char *method, const char *path, route_handler_t handler);

// Register static directory to serve
void register_static_directory(router_t *router, const char *static_dir);

// Route dispatcher
int router_dispatch(router_t *router, struct MHD_Connection *connection,
                    const char *url,
                    const char *method,
                    const char *version,
                    const char *upload_data,
                    size_t *upload_data_size,
                    void **con_cls);

#endif /* ROUTER_H */