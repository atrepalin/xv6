#include "types.h"

void
memcpy_fast(void *dst, const void *src, uint n)
{
    uint cnt = n / 4;
    uint rem = n % 4;

    __asm__ volatile(
        "cld\n\t"
        "rep movsl\n\t"
        : "+D"(dst), "+S"(src), "+c"(cnt)
        :
        : "memory"
    );

    __asm__ volatile(
        "rep movsb\n\t"
        : "+D"(dst), "+S"(src), "+c"(rem)
        :
        : "memory"
    );
}

void
memset_fast(void *dst, uchar c, uint n)
{
    uint cnt = n / 4;
    uint rem = n % 4;
    ulong val = c;
    val = val | (val << 8) | (val << 16) | (val << 24);
    
    __asm__ volatile(
        "cld\n\t"
        "rep stosl\n\t"
        : "+D"(dst), "+c"(cnt)
        : "a"(val)
        : "memory"
    );

    __asm__ volatile(
        "rep stosb\n\t"
        : "+D"(dst), "+c"(rem)
        : "a"(c)
        : "memory"
    );
}

void
memset_fast_long(void *dst, ulong val, uint n)
{
    __asm__ volatile(
        "cld\n\t"
        "rep stosl\n\t"
        : "+D"(dst), "+c"(n)
        : "a"(val)
        : "memory"
    );
}