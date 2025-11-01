#ifndef _XV6_STDLIB_H_
#define _XV6_STDLIB_H_

#include "string.h"

extern void* kmalloc(int size);
extern void kmfree(void *ptr);

static inline int atoi(const char *s) {
    int n = 0, neg = 0;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s++ - '0');
    }
    return neg ? -n : n;
}

static inline void *malloc(size_t size) {
    return kmalloc(size);
}

static inline void free(void *ptr) {
    if (ptr)
        kmfree(ptr);
}

#endif /* _XV6_STDLIB_H_ */
