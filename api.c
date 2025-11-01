#include "types.h"
#include "user.h"
#include "curl.h"
#include "http_parser.h"

#define printf(...) printf(1, __VA_ARGS__)

#define MAX_OUTPUT 2048

int main() {
    struct request req;
    static char output[MAX_OUTPUT];

    req.url = "jsonplaceholder.typicode.com";

    const char *json_body = "{\"title\":\"Hello XV6\",\"body\":\"This is a test\",\"userId\":1}";
    size_t content_length = strlen(json_body);
    const char *content_type = "application/json";

    int ret = curl(
        URL,              
        &req,
        "POST",           
        "/posts",         
        output,           
        json_body,        
        content_length,   
        content_type      
    );

    if (ret != 0) {
        printf("curl failed with code %d (%s)\n", ret, curl_strerror(ret));
        return 1;
    }

    struct http_response *resp = parse_http_response(output);

    printf("Status code: %d\n", resp->status_code);
    printf("Status text: %s\n", resp->status_text);
    printf("Headers:\n");

    for(int i = 0; i < resp->header_count; i++)
        printf("    %s: %s\n", resp->headers[i].name, resp->headers[i].value);

    printf("Body:\n%s\n", resp->body);

    exit();
}
