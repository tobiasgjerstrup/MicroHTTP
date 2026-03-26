#include <stdio.h>

void handle_hello(int client_fd, const char *body, size_t body_len);

void handle_health(int client_fd, const char *body, size_t body_len);

void handle_data(int client_fd, const char *body, size_t body_len);