#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

typedef void (*RouteHandler)(int client_fd);

typedef struct
{
    const char *method;
    const char *path;
    RouteHandler handler;
} Route;

void handle_hello(int client_fd) {
    const char *response = "Hello, World!";
    send(client_fd, response, strlen(response), 0);
}

void handle_health(int client_fd) {
    const char *response = "Very healthy!";
    send(client_fd, response, strlen(response), 0);
}

void handle_data(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read < 0) {
        perror("recv");
        return;
    }
    buffer[bytes_read] = '\0'; // Null-terminate the buffer
    printf("Received data: %s\n", buffer);

    const char *response = "Data received!";
    send(client_fd, response, strlen(response), 0);
}

Route routes[] = {
    {"GET", "/hello", handle_hello},
    {"GET", "/health", handle_health},
    {"POST", "/data", handle_data},
    {NULL, NULL, NULL} // End of routes
};
const int route_count = 3; // Exclude the NULL entry

void dispatch_request(int client_fd, const char *method, const char *path) {
    int path_matched = 0;

    for (int i = 0; i < route_count; i++) {
        if (strcmp(routes[i].method, method) == 0) {
            path_matched = 1;
            if (strcmp(routes[i].path, path) == 0) {
                routes[i].handler(client_fd);
                return;
            }
        }
    }

    if (path_matched) {
        const char *response = "405 Method Not Allowed";
        send(client_fd, response, strlen(response), 0);
    } else {
        const char *response = "404 Not Found";
        send(client_fd, response, strlen(response), 0);
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

    printf("Server is listening on port 8081...\n");

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
        if (parsed != 2) {
            const char *response = "400 Bad Request";
            send(client_fd, response, strlen(response), 0);
            close(client_fd);
            continue;
        }

        dispatch_request(client_fd, method, path);

        // Close the client connection
        close(client_fd);
    }

    return 0;
}
