#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


int main(void) {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[1024];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // Set up server address
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen on all interfaces
    server_addr.sin_port = htons(8081);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return 1;
    }

    // Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        return 1;
    }

    printf("Server is listening on port 8081...\n");

    while (1) {
        // Accept a client connection
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Read data from the client
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read < 0) {
            perror("recv");
            close(client_fd);
            continue;
        }

        buffer[bytes_read] = '\0'; // Null-terminate the buffer
        printf("Received from client: %s\n", buffer);

        // Send a response back to the client
        const char *response = "Hello from the server!";
        send(client_fd, response, strlen(response), 0);

        // Close the client connection
        close(client_fd);
    }

    return 0;
}