#ifndef _XV6_STDDEF_H_
#define _XV6_STDDEF_H_

typedef long ptrdiff_t;
typedef unsigned long size_t;

/* NULL */
#ifndef NULL
#define NULL ((void *)0)
#endif

#define offsetof(type, member) ((size_t)&(((type *)0)->member))

#endif /* _XV6_STDDEF_H_ */
