#ifndef _XV6_CTYPE_H_
#define _XV6_CTYPE_H_

static inline int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

static inline int isxdigit(int c) {
    return ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F'));
}

static inline int isalpha(int c) {
    return ((c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z'));
}

static inline int isspace(int c) {
    return (c == ' ' || c == '\t' || c == '\n' ||
            c == '\r' || c == '\v' || c == '\f');
}

static inline int isprint(int c) {
    return (c >= 32 && c < 127);
}

static inline int isupper(int c) {
    return (c >= 'A' && c <= 'Z');
}

static inline int islower(int c) {
    return (c >= 'a' && c <= 'z');
}

static inline int tolower(int c) {
    return isupper(c) ? (c + 32) : c;
}

static inline int toupper(int c) {
    return islower(c) ? (c - 32) : c;
}

#endif /* _XV6_CTYPE_H_ */
