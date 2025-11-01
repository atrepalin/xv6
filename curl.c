#include "types.h"
#include "user.h"
#include "curl.h"
#include "http_parser.h"

#define printf(...) printf(1, __VA_ARGS__)
#define HEADERS 8

static void strncpy(char *dst, const char *src, int n) {
    int i = 0;
    for (; i < n - 1 && src[i]; i++)
        dst[i] = src[i];
    dst[i] = 0;
}

struct header {
    char name[64];
    char value[128];
};

struct curl_opts {
    char url[256];
    char method[16];
    char path[128];
    char host[64];
    int port;
    char *body;
    char *content_type;
    struct header headers[HEADERS];
    int header_count;
};

static void usage(void);
static void parse_url(const char *url, struct curl_opts *opts);
static void parse_args(int argc, char *argv[], struct curl_opts *opts);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        exit();
    }

    struct curl_opts opts;
    memset(&opts, 0, sizeof(opts));
    strcpy(opts.method, "GET");

    parse_args(argc, argv, &opts);
    parse_url(opts.url, &opts);

    printf("=== CURL DEBUG ===\n");
    printf("URL: %s\n", opts.url);
    printf("Method: %s\n", opts.method);
    printf("Host: %s\n", opts.host);
    printf("Port: %d\n", opts.port);
    printf("Path: %s\n", opts.path);
    if (opts.body)
        printf("Body: %s\n", opts.body);
    if (opts.content_type)
        printf("Content-Type: %s\n", opts.content_type);
    printf("==================\n\n");

    struct request req;
    memset(&req, 0, sizeof(req));

    int curl_type = IP;

    if (!strncmp(opts.url, "http://", 7) || !strncmp(opts.url, "https://", 8)) {
        curl_type = URL;
        req.url = opts.url;
    } else {
        int is_ip = 1;
        for (int i = 0; opts.host[i]; i++) {
            char c = opts.host[i];
            if (!((c >= '0' && c <= '9') || c == '.')) {
                is_ip = 0;
                break;
            }
        }

        if (is_ip) {
            curl_type = IP;
            char *s = (char*)opts.host;
            int a = atoi(s);
            s = strchr(s, '.'); if (!s) goto bad_ip; s++;
            int b = atoi(s);
            s = strchr(s, '.'); if (!s) goto bad_ip; s++;
            int c = atoi(s);
            s = strchr(s, '.'); if (!s) goto bad_ip; s++;
            int d = atoi(s);

            req.ip.a = a;
            req.ip.b = b;
            req.ip.c = c;
            req.ip.d = d;
            req.ip.port = opts.port;
        } else {
            curl_type = URL;
            req.url = opts.url;
        }
    }

    static char buf[4096];
    int err = curl(curl_type, &req, opts.method, opts.path,
                   buf,
                   opts.body, opts.body ? strlen(opts.body) : 0,
                   opts.content_type);

    if (err < 0) {
        printf("curl failed (code %d: %s)\n", err, curl_strerror(err));
        exit();
    }

    struct http_response *res = parse_http_response(buf);
    printf("HTTP %d %s\n", res->status_code, res->status_text);
    for (int i = 0; i < res->header_count; i++)
        printf("%s: %s\n", res->headers[i].name, res->headers[i].value);

    printf("\n%s\n", res->body);
    exit();

bad_ip:
    printf("Invalid IP format: %s\n", opts.host);
    exit();
}

static void usage(void) {
    printf("Usage: curl <url> [-X METHOD] [-d DATA] [-H HEADER]\n");
    printf("Example:\n");
    printf("  curl http://10.0.2.1:8080/test\n");
    printf("  curl example.com\n");
    printf("  curl http://10.0.2.1:8080/greet/world -X POST\n");
    printf("  curl 10.0.2.1:8080/json -X POST -d {\"name\":\"world\"} -H \"Content-Type:application/json\"\n");
}

static void parse_args(int argc, char *argv[], struct curl_opts *opts) {
    strcpy(opts->url, argv[1]);

    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "-X") && i + 1 < argc) {
            strcpy(opts->method, argv[++i]);
        }
        else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
            opts->body = argv[++i];
            if (!strcmp(opts->method, "GET"))
                strcpy(opts->method, "POST");
        }
        else if (!strcmp(argv[i], "-H") && i + 1 < argc) {
            if (opts->header_count < HEADERS) {
                char *h = argv[++i];
                char *colon = strchr(h, ':');
                if (colon) {
                    *colon = 0;
                    strcpy(opts->headers[opts->header_count].name, h);
                    strcpy(opts->headers[opts->header_count].value, colon + 1);
                    char *v = opts->headers[opts->header_count].value;
                    while (*v == ' ') v++;
                    if (!strcmp(opts->headers[opts->header_count].name, "Content-Type"))
                        opts->content_type = v;
                    opts->header_count++;
                }
            }
        }
        else {
            printf("Unknown argument: %s\n", argv[i]);
            usage();
            exit();
        }
    }
}

static void parse_url(const char *url, struct curl_opts *opts) {
    const char *p = url;
    const char *host_start = p;

    if (strncmp(p, "http://", 7) == 0) {
        host_start = p + 7;
    } else if (strncmp(p, "https://", 8) == 0) {
        printf("Warning: HTTPS not supported, using HTTP instead.\n");
        host_start = p + 8;
    }

    const char *path_start = strchr(host_start, '/');
    if (path_start) {
        int len = path_start - host_start;
        strncpy(opts->host, host_start, len + 1);
        strcpy(opts->path, path_start);
    } else {
        strcpy(opts->host, host_start);
        strcpy(opts->path, "/");
    }

    char *colon = strchr(opts->host, ':');
    if (colon) {
        *colon = 0;
        opts->port = atoi(colon + 1);
        if (opts->port <= 0) opts->port = 80;
    } else {
        opts->port = 80;
    }

    if (opts->path[0] == 0)
        strcpy(opts->path, "/");

    strcpy(opts->url, url);
}
