#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
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
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[1024];
    const route_handler routes[] = {
        {"GET", "/hello", handle_hello},
        {"GET", "/health", handle_health},
        {"POST", "/data", handle_data},
    };

    register_routes(routes, sizeof(routes) / sizeof(routes[0]));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("Server is listening on port 8080...\n");

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
            const char *response = "400 Bad Request";
            send_http_response(client_fd, "400 Bad Request", "text/plain; charset=utf-8", response);
            close(client_fd);
            continue;
        }

        dispatch_request(client_fd, method, path);

        close(client_fd);
    }

    return 0;
}
