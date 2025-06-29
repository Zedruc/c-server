#include <stdio.h>
#include <stdlib.h>
#include <microhttpd.h>
#include "router.h"
#include "routes/api/24scope/discord_auth.h"
#include "routes/api/24scope/scope_auth.h"
#include "db_connection.h"
#include "user_db.h"
#include "routes/api/24scope/aircraft_data.h"
#include "env_reader.h"

#define PORT 80

int main() {
    struct MHD_Daemon *d;
    router_t *router = router_init();

    // Register routes
    // router_register(router, "GET", "/api/users/", handle_user_by_id);  // Register "/api/users/{id}" route
    router_register(router, "GET", "/api/24scope/login", handle_scope_login);
    router_register(router, "GET", "/24scope/discord_auth", handle_discord_oauth_redirect);
    router_register(router, "GET", "/24scopebeta/login", handle_scope_login_verification);
    router_register(router, "GET", "/24scope/aircraft-data", handle_aircraft_data_get);
    router_register(router, "POST", "/24scope/aircraft-data", handle_aircraft_data_post);

    register_static_directory(router, "/");

    // Start the HTTP daemon
    d = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, PORT, NULL, NULL, 
                         (MHD_AccessHandlerCallback)router_dispatch, router, MHD_OPTION_END);
    if (d == NULL) {
        fprintf(stderr, "Error starting the daemon\n");
        return 1;
    }

    // read .env file
    (void) env_init();

    // initialize db for user
    char *DB_FILE = strdup(env_get("DB_PATH"));
    if (!DB_FILE)
    {
      fprintf(stderr, "DB_PATH not found\n");
      exit(1);
    }

    (void) db_init(DB_FILE);
    free(DB_FILE);

    printf("Server started on port %d\n", PORT);
    (void) getc (stdin);
    MHD_stop_daemon(d);
    (void) db_close();
    (void) env_close();
    return 0;
}
