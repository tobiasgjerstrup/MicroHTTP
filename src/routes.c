#include "routes.h"
#include "tcp.h"

void handle_hello(int client_fd, const char *body, size_t body_len)
{
    (void)body;
    (void)body_len;

    const char *response = "Hello, world!";
    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", response);
}

void handle_health(int client_fd, const char *body, size_t body_len)
{
    (void)body;
    (void)body_len;

    const char *response = "Very healthy!";
    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", response);
}

void handle_data(int client_fd, const char *body, size_t body_len)
{
    printf("Received data (%zu bytes): %.*s\n", body_len, (int)body_len, body ? body : "");

    const char *response = "Data received!";
    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", response);
}