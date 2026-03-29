# MicroHTTP

MicroHTTP is a small C library for serving HTTP requests with a route table and callback handlers.

It includes a small JSON parser that can either:

- validate arbitrary JSON input
- map JSON object fields into a C struct via field specs

## Build

Build the library artifacts:

```sh
make
```

This produces:

- `libmicrohttp.a`
- `libmicrohttp.so`
- versioned shared library symlinks
- `microhttp.pc`

Build the example application:

```sh
make demo
```

Run the example server:

```sh
make run
```

## Install

Install to the default prefix:

```sh
make install
```

Install to a custom prefix:

```sh
make install PREFIX=/usr
```

Stage an install for packaging:

```sh
make install DESTDIR=/tmp/pkgroot
```

Installed files include:

- `include/microhttp.h`
- `lib/libmicrohttp.a`
- `lib/libmicrohttp.so`
- `lib/pkgconfig/microhttp.pc`

Remove an installed package:

```sh
make uninstall PREFIX=/usr/local
```

## Use From Another Project

After installation, use `pkg-config` to discover the compiler and linker flags.

Compiler flags:

```sh
pkg-config --cflags microhttp
```

Linker flags:

```sh
pkg-config --libs microhttp
```

Example compile command:

```sh
cc app.c $(pkg-config --cflags --libs microhttp) -o app
```

If the package is installed into a non-standard prefix, point `pkg-config` at the generated metadata:

```sh
export PKG_CONFIG_PATH=/custom/prefix/lib/pkgconfig:$PKG_CONFIG_PATH
```

### CMake find_package (Config Mode)

MicroHTTP installs CMake package metadata so consumers can use `find_package`.

Example `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.15)
project(consumer C)

find_package(MicroHTTP CONFIG REQUIRED)

add_executable(consumer_app app.c)
target_link_libraries(consumer_app PRIVATE MicroHTTP::microhttp)
```

If MicroHTTP is installed into a non-standard prefix, configure your project with:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/custom/prefix
```

Installed CMake package files:

- `lib/cmake/MicroHTTP/MicroHTTPConfig.cmake`
- `lib/cmake/MicroHTTP/MicroHTTPConfigVersion.cmake`

## Minimal Example

```c
#include "microhttp.h"

static void handle_hello(int client_fd, const char *body, size_t body_len)
{
    (void)body;
    (void)body_len;

    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", "Hello, world!");
}

static void handle_echo_body(int client_fd, const char *body, size_t body_len)
{
    char response[256];
    snprintf(response, sizeof(response), "received %zu bytes", body_len);
    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", response);
}

int main(void)
{
    const route_handler routes[] = {
        {"GET", "/hello", handle_hello},
        {"POST", "/echo", handle_echo_body},
    };

    register_routes(routes, sizeof(routes) / sizeof(routes[0]));
    return microhttp_serve(8080);
}
```

Then build it with:

```sh
cc app.c $(pkg-config --cflags --libs microhttp) -o app
```

## Public API

The public header is in `include/microhttp.h` and currently exposes:

- `route_handler`
- `JsonFieldType`
- `JsonFieldSpec`
- `register_routes`
- `dispatch_request`
- `send_http_response`
- `microhttp_serve`
- `parse_json`

## JSON Parsing

`parse_json` supports two modes:

1. Validation-only: pass `NULL` for `out_struct` and `fields` and `0` for `field_count`.
2. Struct mapping: pass an output struct pointer and a `JsonFieldSpec` array.

Function signature:

```c
int parse_json(const char *json,
               void *out_struct,
               const JsonFieldSpec *fields,
               size_t field_count,
               const char **error_out);
```

Example mapping JSON into a struct:

```c
#include <stddef.h>
#include <stdio.h>
#include "microhttp.h"

typedef struct {
    char name[64];
    int age;
    char gender[32];
    int active;
} UserProfile;

int main(void)
{
    const char *json = "{\"name\":\"Tobias\",\"age\":25,\"gender\":\"man\",\"active\":true}";
    const char *error = NULL;
    UserProfile user = {0};

    const JsonFieldSpec fields[] = {
        {"name", JSON_FIELD_STRING, offsetof(UserProfile, name), sizeof(user.name), 1},
        {"age", JSON_FIELD_INT, offsetof(UserProfile, age), 0, 1},
        {"gender", JSON_FIELD_STRING, offsetof(UserProfile, gender), sizeof(user.gender), 1},
        {"active", JSON_FIELD_BOOL, offsetof(UserProfile, active), 0, 0},
    };

    if (parse_json(json, &user, fields, sizeof(fields) / sizeof(fields[0]), &error)) {
        printf("name=%s age=%d gender=%s active=%d\n", user.name, user.age, user.gender, user.active);
    } else {
        printf("parse error: %s\n", error ? error : "unknown error");
    }

    return 0;
}
```

Quick test against the example server:

```sh
curl -i -X POST http://localhost:8080/json \
  -H "Content-Type: application/json" \
  -d '{"name":"Tobias","age":25,"gender":"man","active":true}'
```

## Notes

- The current server API is process-global and uses a single registered route table.
- `microhttp_serve` runs a blocking accept loop.
- This project currently targets Unix-like systems.