#include "microhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_REQUEST_SIZE (64 * 1024)

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

static size_t parse_content_length(const char *headers)
{
    const char *line = headers;

    while (line && *line)
    {
        const char *line_end = strstr(line, "\r\n");
        size_t line_len;

        if (!line_end)
        {
            break;
        }

        line_len = (size_t)(line_end - line);
        if (line_len == 0)
        {
            break;
        }

        if (line_len > 15 && strncasecmp(line, "Content-Length:", 15) == 0)
        {
            const char *value = line + 15;
            while (*value == ' ' || *value == '\t')
            {
                value++;
            }
            return (size_t)strtoul(value, NULL, 10);
        }

        line = line_end + 2;
    }

    return 0;
}

void dispatch_request(int client_fd,
                      const char *method,
                      const char *path,
                      const char *body,
                      size_t body_len)
{
    int path_matched = 0;

    for (size_t i = 0; i < registered_route_count; i++)
    {
        if (strcmp(registered_routes[i].path, path) == 0)
        {
            path_matched = 1;
            if (strcmp(registered_routes[i].method, method) == 0)
            {
                registered_routes[i].handler(client_fd, body, body_len);
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
    char *buffer = malloc(MAX_REQUEST_SIZE + 1);

    if (!buffer)
    {
        perror("malloc");
        return 1;
    }

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

        size_t total_read = 0;
        size_t content_length = 0;
        size_t header_len = 0;
        const char *body = NULL;
        size_t body_len = 0;
        int headers_ready = 0;

        while (total_read < MAX_REQUEST_SIZE)
        {
            ssize_t bytes_read = recv(client_fd,
                                      buffer + total_read,
                                      MAX_REQUEST_SIZE - total_read,
                                      0);

            if (bytes_read < 0)
            {
                perror("recv");
                close(client_fd);
                goto next_client;
            }

            if (bytes_read == 0)
            {
                break;
            }

            total_read += (size_t)bytes_read;
            buffer[total_read] = '\0';

            if (!headers_ready)
            {
                char *header_end = strstr(buffer, "\r\n\r\n");
                if (!header_end)
                {
                    continue;
                }

                headers_ready = 1;
                header_len = (size_t)(header_end - buffer) + 4;
                content_length = parse_content_length(buffer);
            }

            if (headers_ready && total_read >= header_len + content_length)
            {
                break;
            }
        }

        if (!headers_ready)
        {
            send_http_response(client_fd, "400 Bad Request", "text/plain; charset=utf-8", "400 Bad Request");
            close(client_fd);
            goto next_client;
        }

        if (total_read < header_len + content_length)
        {
            send_http_response(client_fd, "400 Bad Request", "text/plain; charset=utf-8", "Incomplete Request Body");
            close(client_fd);
            goto next_client;
        }

        body = buffer + header_len;
        body_len = content_length;

        char method[16], path[256];
        int parsed = sscanf(buffer, "%15s %255s", method, path);
        if (parsed != 2)
        {
            send_http_response(client_fd, "400 Bad Request", "text/plain; charset=utf-8", "400 Bad Request");
            close(client_fd);
            goto next_client;
        }

        dispatch_request(client_fd, method, path, body, body_len);
        close(client_fd);

    next_client:
        ;
    }

    free(buffer);
    close(server_fd);
    return 0;
}