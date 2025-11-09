#include "types.h"
#include "user.h"
#include "http.h"
#include "http_parser.h"

int main(int argc, char *argv[]) {
    char req[HTTP_BUF_SIZE];

    read_buffer(stdin, req);

    struct http_request *request = parse_http_request(req);

    printf(stderr, "Method: %s\n", request->method);
    printf(stderr, "Uri: %s\n", request->uri);
    printf(stderr, "Headers:\n");

    for (int i = 0; i < request->header_count; i++)
        printf(stderr, "    %s: %s\n", request->headers[i].name, request->headers[i].value);

    printf(stderr, "Body:\n%s\n", request->body);

    const char *body = "Hello from xv6!\n";

    printf(stdout, "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body), body);

    exit();
}
