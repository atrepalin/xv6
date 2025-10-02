#include "types.h"
#include "x86.h"
#include "defs.h"
#include "spinlock.h"
#include "traps.h"
#include "msg.h"
#include "param.h"
#include "mmu.h"
#include "fs.h"
#include "window.h"
#include "proc.h"

int chan = 0;

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

void wait_for_msg(struct proc *p) {
    acquire(&ptable.lock);
    p->chan = &chan;
    p->state = SLEEPING;

    int intena = mycpu()->intena;
    swtch(&p->context, mycpu()->scheduler);
    mycpu()->intena = intena;

    release(&ptable.lock);

    p->chan = 0;
}

void wakeup_on_msg(struct proc *p) {
    if(p->state == SLEEPING && p->chan == &chan) {
        p->state = RUNNABLE;
        p->chan = 0;
    }
}

extern struct window *tail;

void enqueue_msg(struct window *win, struct msg *m) {
    if(!win) return;

    if (m->type > M_KEY_DOWN) {
        m->mouse.x -= win->x;
        m->mouse.y -= win->y;

        if(m->mouse.x < 0 || m->mouse.x >= win->w ||
            m->mouse.y < 0 || m->mouse.y >= win->h) return;
    }

    int next = (win->queue.tail + 1) % MSG_QUEUE_SIZE;

    if (next != win->queue.head) {
        win->queue.msgs[win->queue.tail] = *m;
        win->queue.tail = next;

        struct proc *p = win->owner;

        wakeup_on_msg(p);
    }
}

void send_msg(struct msg *msg) {
    if(msg->type == M_MOUSE_DOWN) {
        if(click_bring_to_front()) return;
    }

    enqueue_msg(tail, msg);
}

int 
sys_get_msg(void) {
    char* user_msg;
    int should_wait = 0;

    if(argptr(0, &user_msg, sizeof(struct msg)) < 0) return -1;
    if(argint(1, &should_wait) < 0) return -1;

    struct proc *p = myproc();
    struct window *win = p->win;
    if (!win) return 0;

    if (win->queue.head == win->queue.tail && should_wait) {
        wait_for_msg(p);
    }

    struct msg m = win->queue.msgs[win->queue.head];
    win->queue.head = (win->queue.head + 1) % MSG_QUEUE_SIZE;

    if (copyout(p->pgdir, (uint)user_msg, (char *)&m, sizeof(m)) < 0)
        return 0;

    return 1;
}