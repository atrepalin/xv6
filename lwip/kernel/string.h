#ifndef _XV6_STRING_H_
#define _XV6_STRING_H_

#include "stdint.h"
#include "stddef.h"

static inline void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dst;
}

static inline void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    if (d < s) {
        while (n--)
            *d++ = *s++;
    } else {
        d += n;
        s += n;
        while (n--)
            *--d = *--s;
    }
    return dst;
}

static inline void *memset(void *dst, int c, size_t n) {
    unsigned char *d = dst;
    while (n--)
        *d++ = (unsigned char)c;
    return dst;
}

static inline int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *p1 = a;
    const unsigned char *p2 = b;
    while (n--) {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

static inline size_t strlen(const char *s) {
    size_t n = 0;
    while (*s++) n++;
    return n;
}

static inline char *strcpy(char *dst, const char *src) {
    char *r = dst;
    while ((*dst++ = *src++));
    return r;
}

static inline int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++; b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

static inline int strncmp(const char *p, const char *q, size_t n) {
    while(n > 0 && *p && *p == *q)
        n--, p++, q++;
    if(n == 0)
        return 0;

    return *(const unsigned char*)p - *(const unsigned char*)q;
}

#endif /* _XV6_STRING_H_ */
