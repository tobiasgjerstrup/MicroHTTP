void send_http_response(int client_fd,
                               const char *status,
                               const char *content_type,
                               const char *body);

void dispatch_request(int client_fd, const char *method, const char *path);