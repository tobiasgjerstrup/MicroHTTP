#include <stdio.h>
#include "microhttp.h"

static void handle_hello(int client_fd, const char *body, size_t body_len)
{
    (void)body;
    (void)body_len;

    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", "Hello, world!");
}

static void handle_health(int client_fd, const char *body, size_t body_len)
{
    (void)body;
    (void)body_len;

    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", "Very healthy!");
}

static void handle_data(int client_fd, const char *body, size_t body_len)
{
    printf("Received data (%zu bytes): %.*s\n", body_len, (int)body_len, body ? body : "");
    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", "Data received!");
}

int main(void)
{
    const route_handler routes[] = {
        {"GET", "/hello", handle_hello},
        {"GET", "/health", handle_health},
        {"POST", "/data", handle_data},
    };

    register_routes(routes, sizeof(routes) / sizeof(routes[0]));
    return microhttp_serve(8080);
}
