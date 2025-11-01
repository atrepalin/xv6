#include "types.h"
#include "user.h"
#include "curl.h"
#include "http_parser.h"

#define printf(...) printf(1, __VA_ARGS__)

int main(int argc, char *argv[]) {
    if (argc < 7) {
        printf("Usage: %s <a> <b> <c> <d> <port> <method> <path>\n", argv[0]);
        exit();
    }

    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    int c = atoi(argv[3]);
    int d = atoi(argv[4]);
    int port = atoi(argv[5]);
    char *method = argv[6];
    char *path = argv[7];

    static char buf[2048];

    int error;

    if ((error = curl(a, b, c, d, port, method, path, buf)) < 0) {
        printf("curl failed (code %d: %s)\n", error, curl_strerror(error));
    }
    else {
        struct http_response *res = parse_http_response(buf);

        printf("Status code: %d\n", res->status_code);
        printf("Status text: %s\n", res->status_text);
        printf("Headers:\n");

        for(int i = 0; i < res->header_count; i++)
            printf("    %s: %s\n", res->headers[i].name, res->headers[i].value);

        printf("Body:\n%s\n", res->body);
    }

    exit();
}
