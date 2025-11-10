#include "types.h"
#include "user.h"
#include "http.h"
#include "http_parser.h"
#include "fcntl.h"

const char *filename = "index.html";
const char *mimetype = "text/html";

int main(int argc, char *argv[]) {
    static char req[HTTP_BUF_SIZE];
    read_buffer(stdin, req);

    struct http_request *request = parse_http_request(req);

    printf(stderr, "Method: %s\n", request->method);
    printf(stderr, "Uri: %s\n", request->uri);
    printf(stderr, "Headers:\n");

    for (int i = 0; i < request->header_count; i++)
        printf(stderr, "    %s: %s\n", request->headers[i].name, request->headers[i].value);

    printf(stderr, "Body:\n%s\n", request->body);

    if (strcmp(request->method, "GET") != 0) {
        const char *err = "Method not allowed\n";
        printf(stdout,
            "HTTP/1.1 405 Method Not Allowed\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(err), err);
        exit();
    }

    if (strcmp(request->uri, "/") != 0) {
        const char *err = "Not found\n";
        printf(stdout,
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n"
            "%s",
            strlen(err), err);
        exit();
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        char err[64];
        snprintf(err, sizeof(err), "Failed to open %s\n", filename);
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

    static char body[HTTP_BUF_SIZE - 192];
    int n = read(fd, body, sizeof(body) - 1);
    close(fd);

    if (n < 0)
        n = 0;
    body[n] = '\0';

    printf(stdout,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Content-Disposition: inline; filename=\"%s\"\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        mimetype, n, filename, body);

    exit();
}
