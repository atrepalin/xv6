#include "types.h"
#include "defs.h"
#include "spinlock.h"
#include "param.h"
#include "mmu.h"
#include "fs.h"
#include "proc.h"

static int chan;

extern struct {
  struct spinlock lock;
} ptable;

void wait_for_msg(struct proc *p) {
    acquire(&ptable.lock);

    p->chan = &chan;
    p->state = SLEEPING;

    int intena = mycpu()->intena;
    swtch(&p->context, mycpu()->scheduler);
    mycpu()->intena = intena;

    p->chan = 0;

    release(&ptable.lock);
}

void wakeup_on_msg(struct proc *p) {
    acquire(&ptable.lock);

    if (p->state == SLEEPING && p->chan == &chan) {
        p->state = RUNNABLE;
        p->chan = 0;
    }

    release(&ptable.lock);
}