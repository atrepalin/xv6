#define MAX_LINE     256
#define MAX_HEADERS  32

struct http_header {
    char name[64];
    char value[128];
};

struct http_response {
    int status_code;
    char status_text[64];
    int header_count;
    struct http_header headers[MAX_HEADERS];
    const char *body;
};

struct http_response* parse_http_response(const char *response);