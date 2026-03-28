#include "microhttp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
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

int microhttp_serve(unsigned short port)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[1024];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server is listening on port %hu...\n", port);

    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read < 0)
        {
            perror("recv");
            close(client_fd);
            continue;
        }

        buffer[bytes_read] = '\0';

        char method[16], path[256];
        int parsed = sscanf(buffer, "%15s %255s", method, path);
        if (parsed != 2)
        {
            send_http_response(client_fd, "400 Bad Request", "text/plain; charset=utf-8", "400 Bad Request");
            close(client_fd);
            continue;
        }

        dispatch_request(client_fd, method, path);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}