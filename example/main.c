#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

typedef struct
{
    char name[64];
    int age;
    char gender[32];
    int active;
} UserProfile;

static void handle_json_input(int client_fd, const char *body, size_t body_len)
{
    const char *parse_error = NULL;
    char *json_body = NULL;
    UserProfile user = {0};
    const JsonFieldSpec fields[] = {
        {"name", JSON_FIELD_STRING, offsetof(UserProfile, name), sizeof(user.name), 1},
        {"age", JSON_FIELD_INT, offsetof(UserProfile, age), 0, 1},
        {"gender", JSON_FIELD_STRING, offsetof(UserProfile, gender), sizeof(user.gender), 1},
        {"active", JSON_FIELD_BOOL, offsetof(UserProfile, active), 0, 0},
    };

    printf("Received data (%zu bytes): %.*s\n", body_len, (int)body_len, body ? body : "");

    json_body = malloc(body_len + 1);
    if (!json_body) {
        send_http_response(client_fd, "500 Internal Server Error", "text/plain; charset=utf-8", "Allocation failure");
        return;
    }

    if (body_len > 0 && body) {
        memcpy(json_body, body, body_len);
    }
    json_body[body_len] = '\0';

    if (parse_json(json_body, &user, fields, sizeof(fields) / sizeof(fields[0]), &parse_error)) {
        char response[256];
        snprintf(response, sizeof(response), "Parsed user: name=%s age=%d gender=%s active=%d", user.name, user.age, user.gender, user.active);
        send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", response);
    } else {
        char response[256];
        snprintf(response, sizeof(response), "JSON parse error: %s", parse_error ? parse_error : "unknown error");
        send_http_response(client_fd, "400 Bad Request", "text/plain; charset=utf-8", response);
    }

    free(json_body);
}

int main(void)
{
    /* const char *json = "{\"name\":\"Tobias\",\"age\":25,\"gender\":\"man\",\"skills\":[\"C\",\"C++\",\"Python\"],\"active\":true,\"meta\":{\"level\":3}}";
    const char *parse_error = NULL;
    UserProfile user = {0};
    const JsonFieldSpec fields[] = {
        {"name", JSON_FIELD_STRING, offsetof(UserProfile, name), sizeof(user.name), 1},
        {"age", JSON_FIELD_INT, offsetof(UserProfile, age), 0, 1},
        {"gender", JSON_FIELD_STRING, offsetof(UserProfile, gender), sizeof(user.gender), 1},
        {"active", JSON_FIELD_BOOL, offsetof(UserProfile, active), 0, 0},
    };

    if (parse_json(json, &user, fields, sizeof(fields) / sizeof(fields[0]), &parse_error)) {
        printf("Struct parse: OK\n");
        printf("name=%s age=%d gender=%s active=%d\n", user.name, user.age, user.gender, user.active);
    } else {
        printf("Struct parse: ERROR (%s)\n", parse_error ? parse_error : "unknown error");
    }

    if (parse_json(json, NULL, NULL, 0, &parse_error)) {
        printf("Generic validation: OK\n");
    } else {
        printf("Generic validation: ERROR (%s)\n", parse_error ? parse_error : "unknown error");
    }
 */
    const route_handler routes[] = {
        {"GET", "/hello", handle_hello},
        {"GET", "/health", handle_health},
        {"POST", "/data", handle_data},
        {"POST", "/json", handle_json_input},
    };

    register_routes(routes, sizeof(routes) / sizeof(routes[0]));
    return microhttp_serve(8080);
}
