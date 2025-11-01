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
static unsigned int lwip_rand_seed = 123456789;

static inline uint32_t lwip_rand(void) {
    lwip_rand_seed = lwip_rand_seed * 1103515245 + 12345;
    return (lwip_rand_seed / 65536) % 32768;
}

#define LWIP_RAND() lwip_rand()

#endif /* _XV6_STDLIB_H_ */
