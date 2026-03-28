#include "microhttp.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

static const route_handler *registered_routes = NULL;
static size_t registered_route_count = 0;

void send_http_response(int client_fd,
                               const char *status,
                               const char *content_type,
                               const char *body)
{
    char header[512];
    int body_len = strlen(body);

    int n = snprintf(header, sizeof(header),
                     "HTTP/1.1 %s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %d\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     status, content_type, body_len);

    send(client_fd, header, n, 0);
    send(client_fd, body, body_len, 0);
}

void register_routes(const route_handler *routes, size_t num_routes)
{
    registered_routes = routes;
    registered_route_count = num_routes;
}

void dispatch_request(int client_fd, const char *method, const char *path)
{
    int path_matched = 0;

    for (size_t i = 0; i < registered_route_count; i++)
    {
        if (strcmp(registered_routes[i].path, path) == 0)
        {
            path_matched = 1;
            if (strcmp(registered_routes[i].method, method) == 0)
            {
                registered_routes[i].handler(client_fd, NULL, 0);
                return;
            }
        }
    }

    if (path_matched)
    {
        const char *response = "405 Method Not Allowed";
        send_http_response(client_fd, "405 Method Not Allowed", "text/plain; charset=utf-8", response);
    }
    else
    {
        const char *response = "404 Not Found";
        send_http_response(client_fd, "404 Not Found", "text/plain; charset=utf-8", response);
    }
}