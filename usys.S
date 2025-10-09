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
SYSCALL(poweroff)

SYSCALL(destroy_window)
SYSCALL(invalidate_window)
SYSCALL(fill_window)
SYSCALL(fill_rect)
SYSCALL(draw_line)
SYSCALL(draw_text)
SYSCALL(bring_to_front)
SYSCALL(move_window)
SYSCALL(get_msg)
SYSCALL(capture_window)
SYSCALL(release_window)

.globl sys_create_window;
  sys_create_window:
    movl $SYS_create_window, %eax;
    int $T_SYSCALL;
    ret