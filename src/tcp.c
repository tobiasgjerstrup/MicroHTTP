#include "tcp.h"
#include <stdio.h>
// #include <stdlib.h>
#include <string.h>
// #include <unistd.h>
#include <arpa/inet.h>
#include "routes.h"

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

typedef void (*RouteHandler)(int client_fd, const char *body, size_t body_len);

typedef struct
{
    const char *method;
    const char *path;
    RouteHandler handler;
} Route;

Route routes[] = {
    {"GET", "/hello", handle_hello},
    {"GET", "/health", handle_health},
    {"POST", "/data", handle_data},
    {NULL, NULL, NULL} // End of routes
};
const int route_count = 3; // Exclude the NULL entry

void dispatch_request(int client_fd, const char *method, const char *path)
{
    int path_matched = 0;

    for (int i = 0; i < route_count; i++)
    {
        if (strcmp(routes[i].method, method) == 0)
        {
            path_matched = 1;
            if (strcmp(routes[i].path, path) == 0)
            {
                routes[i].handler(client_fd, NULL, 0);
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