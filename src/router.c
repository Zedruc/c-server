#include "router.h"
#include "static_handler.h"
#include "post_iterator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Initialize a router
router_t *router_init()
{
  router_t *router = malloc(sizeof(router_t));
  if (!router)
  {
    return NULL;
  }
  router->routes = NULL;
  router->route_count = 0;
  router->static_path = NULL;
  router->static_content_handler = NULL;
  return router;
}

// Register a route to the router
void router_register(router_t *router, const char *method, const char *path, route_handler_t handler)
{
  router->route_count++;
  router->routes = realloc(router->routes, router->route_count * sizeof(route_t));
  router->routes[router->route_count - 1].method = strdup(method);
  router->routes[router->route_count - 1].path = strdup(path);
  router->routes[router->route_count - 1].handler = handler;
}

void register_static_directory(router_t *router, const char *root_path)
{
  // Register the a static route to serve local files from e.g. websites
  // router_register(router, root_path, static_handler);
  router->static_path = root_path;
  router->static_content_handler = static_handler;
}

// Dispatch a request to the right route handler
// This is the MHD initial callback function
int router_dispatch(router_t *router, struct MHD_Connection *connection,
                    const char *url,
                    const char *method,
                    const char *version,
                    const char *upload_data,
                    size_t *upload_data_size,
                    void **con_cls)
{
  for (size_t i = 0; i < router->route_count; i++)
  {
    // printf("%s, %s\n", router->routes[i].method, method);
    if (strncmp(url, router->routes[i].path, strlen(router->routes[i].path)) == 0 && strcmp(router->routes[i].method, method) == 0)
    {
      // Route found, dispatch to the handler

      int matches = strcmp(router->routes[i].method, "POST");
      if (matches == 0)
      {
        const char *content_type = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "content-type");

        if (strcmp(content_type, "application/json") != 0)
        {
          return router->routes[i].handler(connection, url, NULL);
        }

        post_data_t *post_data = NULL;
        int result = get_post_body(&post_data, connection, url, method, version, upload_data, upload_data_size, con_cls);

        if (result == MHD_YES || *upload_data_size != 0)
        {
          // Still receiving data
          return result;
        }

        if (post_data == NULL)
        {
          fprintf(stderr, "Failed to receive POST data\n");
          return MHD_NO;
        }

        // You can now pass post_data to your handler
        // return router->routes[i].handler(connection, url, post_data);
        // (Adjust handler signature accordingly)

        // For now, just free post_data since you're not using it:
        // free(post_data);

        return router->routes[i].handler(connection, url, post_data);
      }
      else
      {
        printf("ah yes not a post\n");
        return router->routes[i].handler(connection, url, NULL);
      }
    }
  }

  // try static handler
  // printf("no route matched\n");
  if (router->static_content_handler != NULL)
  {
    // printf("attempting static content handler\n");
    return router->static_content_handler(connection, url, NULL);
  }
  printf("default 404\n");
  // Default 404
  const char *not_found = "404 Not Found";
  struct MHD_Response *response = MHD_create_response_from_buffer(strlen(not_found), (void *)not_found, MHD_RESPMEM_PERSISTENT);
  int ret = MHD_queue_response(connection, MHD_HTTP_NOT_FOUND, response);
  MHD_destroy_response(response);
  return ret;

  // return MHD_NO; // No matching route
}
