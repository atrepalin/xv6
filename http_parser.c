#include "types.h"
#include "user.h"
#include "http_parser.h"

static int isdigit(char c) {
    return c >= '0' && c <= '9';
}

static void strncpy(char *dst, const char *src, int n) {
    int i = 0;
    for (; i < n - 1 && src[i]; i++)
        dst[i] = src[i];
    dst[i] = 0;
}

static void strtrim(char *s) {
    int start = 0;
    while (s[start] == ' ' || s[start] == '\t') start++;

    int end = 0;
    while (s[end]) end++;
    while (end > start && 
           (s[end - 1] == ' ' || s[end - 1] == '\t' ||
            s[end - 1] == '\r' || s[end - 1] == '\n'))
        end--;

    int i = 0;
    while (start < end)
        s[i++] = s[start++];
    s[i] = 0;
}

static const char *find_line_end(const char *s) {
    int i = 0;
    while (s[i]) {
        if (s[i] == '\n') return s + i;
        i++;
    }
    return 0;
}

struct http_response* parse_http_response(const char *response) {
    static struct http_response res;
    res.header_count = 0;
    res.body = 0;
    res.status_code = 0;
    res.status_text[0] = 0;

    if (!response)
        return &res;

    const char *pos = response;
    char line[MAX_LINE];
    const char *line_end = find_line_end(pos);
    if (!line_end)
        return &res;

    int len = line_end - pos;
    if (len >= MAX_LINE) len = MAX_LINE - 1;
    for (int i = 0; i < len; i++) line[i] = pos[i];
    line[len] = 0;

    int i = 0;
    while (line[i] && line[i] != ' ') i++;
    if (line[i] == ' ') i++;

    res.status_code = 0;
    while (isdigit(line[i])) {
        res.status_code = res.status_code * 10 + (line[i] - '0');
        i++;
    }

    while (line[i] == ' ') i++;
    strncpy(res.status_text, line + i, sizeof(res.status_text));
    strtrim(res.status_text);

    pos = line_end + 1;

    while (1) {
        if (pos[0] == '\r' && pos[1] == '\n') { pos += 2; break; }
        if (pos[0] == '\n') { pos++; break; }
        if (!pos[0]) break;

        line_end = find_line_end(pos);
        if (!line_end) break;
        len = line_end - pos;
        if (len >= MAX_LINE) len = MAX_LINE - 1;
        for (i = 0; i < len; i++) line[i] = pos[i];
        line[len] = 0;

        int sep = 0;
        while (line[sep] && line[sep] != ':') sep++;
        if (line[sep] == ':' && res.header_count < MAX_HEADERS) {
            line[sep] = 0;
            char *name = line;
            char *value = line + sep + 1;

            strtrim(name);
            strtrim(value);

            strncpy(res.headers[res.header_count].name, name, sizeof(res.headers[res.header_count].name));
            strncpy(res.headers[res.header_count].value, value, sizeof(res.headers[res.header_count].value));

            res.header_count++;
        }

        pos = line_end + 1;
    }

    res.body = pos;

    return &res;
}
