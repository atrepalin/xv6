#include "types.h"
#include "user.h"
#include "http.h"
#include "http_parser.h"
#include "fcntl.h"

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

    int fd = open("main.html", O_RDONLY);
    if (fd < 0) {
        const char *err = "Failed to open main.html\n";
        printf(stdout,
            "HTTP/1.1 500 Internal Server Error\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(err), err);
        exit();
    }

    static char body[512];
    int n = read(fd, body, sizeof(body) - 1);
    close(fd);

    if (n < 0)
        n = 0;
    body[n] = '\0';

    printf(stdout,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        n, body);

    exit();
}
