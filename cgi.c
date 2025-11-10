#include "types.h"
#include "user.h"
#include "http.h"
#include "http_parser.h"

#define printf(...) printf(1, __VA_ARGS__)
#define scanf(...) scanf(0, __VA_ARGS__)

int main(int argc, char *argv[]) {
    uint8_t ip[4];
    get_ip(ip);

    int port;
    char path[64];

    if (argc > 1) {
        port = atoi(argv[1]);
    } else {
        printf("Enter port: ");
        scanf("%d", &port);
    }

    if (argc > 2) {
        strcpy(path, argv[2]);
    } else {
        printf("Enter path: ");
        scanf("%s", path);
        strtrim(path);
    }

    int ret = httpd_init(port);

    if (ret != ERR_OK) {
        printf("httpd failed with code %d (%s)\n", ret, strerror(ret));
        
        exit();
    } else {
        printf("httpd listening on %d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], port);
    }

    while ((ret = httpd_poll(cgi_handler, path)) == ERR_OK);

    printf("httpd failed with code %d (%s)\n", ret, strerror(ret));

    exit();
}
