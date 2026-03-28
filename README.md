# MicroHTTP

MicroHTTP is a small C library for serving HTTP requests with a route table and callback handlers.

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

## Minimal Example

```c
#include "microhttp.h"

static void handle_hello(int client_fd, const char *body, size_t body_len)
{
    (void)body;
    (void)body_len;

    send_http_response(client_fd, "200 OK", "text/plain; charset=utf-8", "Hello, world!");
}

int main(void)
{
    const route_handler routes[] = {
        {"GET", "/hello", handle_hello},
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
- `register_routes`
- `dispatch_request`
- `send_http_response`
- `microhttp_serve`

## Notes

- The current server API is process-global and uses a single registered route table.
- `microhttp_serve` runs a blocking accept loop.
- This project currently targets Unix-like systems.