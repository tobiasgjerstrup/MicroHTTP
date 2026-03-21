#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef void (*RouteHandler)(int client_fd, const char *body, size_t body_len);

typedef struct
{
    const char *method;
    const char *path;
    RouteHandler handler;
} Route;

static void send_http_response(int client_fd,
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

void handle_hello(int client_fd, const char *body, size_t body_len)
{
    const char *response = "Hello, World!";
    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", response);
}

void handle_health(int client_fd, const char *body, size_t body_len)
{
    const char *response = "Very healthy!";
    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", response);
}

void handle_data(int client_fd, const char *body, size_t body_len)
{
    printf("Received data (%zu bytes): %.*s\n", body_len, (int)body_len, body ? body : "");

    const char *response = "Data received!";
    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", response);
}

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

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[1024];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
    {
        perror("socket");
        return 1;
    }

    // Set up server address
    server_addr.sin_family = AF_INET;         // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(8080);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind");
        return 1;
    }

    // Listen for connections
    if (listen(server_fd, 5) < 0)
    {
        perror("listen");
        return 1;
    }

    printf("Server is listening on port 8080...\n");

    while (1)
    {
        // Accept a client connection
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0)
        {
            perror("accept");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Read data from the client
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read < 0)
        {
            perror("recv");
            close(client_fd);
            continue;
        }

        buffer[bytes_read] = '\0'; // Null-terminate the buffer
        printf("Received from client: %s\n", buffer);

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

        // Close the client connection
        close(client_fd);
    }

    return 0;
}
