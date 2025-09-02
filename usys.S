#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)

SYSCALL(vbe_init)
SYSCALL(vbe_disable)
SYSCALL(vbe_clear)
SYSCALL(vbe_putpixel)
SYSCALL(vbe_draw_triangle)
SYSCALL(vbe_draw_circle)
SYSCALL(vbe_draw_rect)
SYSCALL(vbe_draw_line)
SYSCALL(vbe_draw_sprite)