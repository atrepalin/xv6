#ifndef __ARCH_SYS_ARCH_H__
#define __ARCH_SYS_ARCH_H__

#include "lwip/arch.h"
#include "lwip/err.h"
#include "lwip/sys.h"

typedef u32_t sys_prot_t;

static inline sys_prot_t sys_arch_protect(void) {
    return 1;
}

static inline void sys_arch_unprotect(sys_prot_t pval) {
    (void)pval;
}

static inline u32_t sys_now(void) {
    extern uint ticks;
    return ticks;
}

static inline void sys_init(void) { }

#endif /* __ARCH_SYS_ARCH_H__ */
