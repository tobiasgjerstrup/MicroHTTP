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

typedef enum
{
	JSON_FIELD_STRING,
	JSON_FIELD_INT,
	JSON_FIELD_BOOL
} JsonFieldType;

typedef struct
{
	const char *key;
	JsonFieldType type;
	size_t offset;
	size_t size;
	int required;
} JsonFieldSpec;

void send_http_response(int client_fd,
						const char *status,
						const char *content_type,
						const char *body);

void dispatch_request(int client_fd,
					  const char *method,
					  const char *path,
					  const char *body,
					  size_t body_len);

void register_routes(const route_handler *routes, size_t num_routes);

int microhttp_serve(unsigned short port);

int parse_json(const char *json,
			   void *out_struct,
			   const JsonFieldSpec *fields,
			   size_t field_count,
			   const char **error_out);

#endif
