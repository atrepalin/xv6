#include "types.h"
#include "user.h"
#include "http.h"
#include "http_parser.h"

#define printf(...) printf(1, __VA_ARGS__)
#define scanf(...) scanf(0, __VA_ARGS__)

void handler(struct incoming_http_request *req, struct outgoing_http_response *resp) {
    struct http_request *request = parse_http_request(req->request);

    printf("Method: %s\n", request->method);
    printf("Uri: %s\n", request->uri);
    printf("Headers:\n");

    for (int i = 0; i < request->header_count; i++)
        printf("    %s: %s\n", request->headers[i].name, request->headers[i].value);

    printf("Body:\n%s\n", request->body);

    char body[256];

    if (strcmp(request->method, "POST") == 0 &&
        strncmp(request->uri, "/greet/", 7) == 0) {

        const char *name = request->uri + 7;

        if (*name)
            snprintf(body, sizeof(body), "Hello, %s!\n", name);
        else
            snprintf(body, sizeof(body), "Hello, stranger!\n");
    } else {
        snprintf(body, sizeof(body), "Hello from xv6!\n");
    }

    int resp_len = snprintf(resp->response, sizeof(resp->response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(body), body);

    resp->total_len = resp_len;
}

int main(int argc, char *argv[]) {
    uint8_t ip[4];
    get_ip(ip);

    int port;

    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        printf("Enter port: ");
        scanf("%d", &port);
    }

    int ret = httpd_init(port);

    if (ret != ERR_OK) {
        printf("httpd failed with code %d (%s)\n", ret, strerror(ret));
        
        exit();
    } else {
        printf("httpd listening on %d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], port);
    }

    while((ret = httpd_poll(handler)) == ERR_OK);

    printf("httpd failed with code %d (%s)\n", ret, strerror(ret));

    exit();
}
