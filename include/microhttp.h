#ifndef MICROHTTP_H
#define MICROHTTP_H

#include <stddef.h>

typedef void (*route_callback)(int client_fd, const char *body, size_t body_len);

typedef struct
{
    const char *method;
    const char *path;
    route_callback handler;
} route_handler;

void send_http_response(int client_fd,
						const char *status,
						const char *content_type,
						const char *body);

void dispatch_request(int client_fd, const char *method, const char *path);

void register_routes(const route_handler *routes, size_t num_routes);

int microhttp_serve(unsigned short port);

void test_parse_person(void);

#endif
