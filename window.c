#include "window.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

int
sys_create_window(void)
{
    int x, y, w, h;
    if (argint(0, &x) < 0) return -1;
    if (argint(1, &y) < 0) return -1;
    if (argint(2, &w) < 0) return -1;
    if (argint(3, &h) < 0) return -1;

    struct proc *p = myproc();
    if (p->win != 0)
        return -1;

    struct window *win = kmalloc(sizeof(struct window));
    if (!win) return -1;

    initlock(&win->lock, "window");
    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->visible = 1;
    win->next_z = 0;
    win->owner = p;

    win->queue.head = win->queue.tail = 0;
    initlock(&win->queue.lock, "msg_queue");

    win->backbuf = kmalloc(w * h * sizeof(struct rgb));
    if (!win->backbuf) {
        kmfree(win);
        return -1;
    }

    memset_fast_long(win->backbuf, 0, w * h);

    add_window(win);
    p->win = win;

    return 0;
}

int
sys_invalidate_window(void)
{
    int x, y, w, h;
    if (argint(0, &x) < 0) return -1;
    if (argint(1, &y) < 0) return -1;
    if (argint(2, &w) < 0) return -1;
    if (argint(3, &h) < 0) return -1;

    struct proc *p = myproc();
    if (!p->win) return -1;

    struct window *win = p->win;
    int gx = win->x + x;
    int gy = win->y + y;

    acquire(&win->lock);
    invalidate(gx, gy, w, h);
    release(&win->lock);
    
    return 0;
}

int
sys_fill_window(void)
{
    int r, g, b;

    if (argint(0, &r) < 0) return -1;
    if (argint(1, &g) < 0) return -1;
    if (argint(2, &b) < 0) return -1;

    struct proc *p = myproc();
    if (!p->win) return -1;

    struct window *win = p->win;

    acquire(&win->lock);
    memset_fast_long(win->backbuf, PACK(r, g, b), win->w * win->h);

    invalidate(win->x, win->y, win->w, win->h);
    release(&win->lock);

    return 0;
}

int
sys_bring_to_front(void) {
    struct proc *p = myproc();
    if (!p->win) return -1;

    struct window *win = p->win;

    acquire(&win->lock);
    bring_to_front(win);

    invalidate(win->x, win->y, win->w, win->h);
    release(&win->lock);

    return 0;
}

int 
sys_move_window(void) {
    int new_x, new_y;

    if (argint(0, &new_x) < 0) return -1;
    if (argint(1, &new_y) < 0) return -1;

    struct proc *p = myproc();
    if (!p->win) return -1;

    struct window *win = p->win;

    acquire(&win->lock);
    invalidate(win->x, win->y, win->w, win->h);

    win->x = new_x;
    win->y = new_y;

    invalidate(win->x, win->y, win->w, win->h);
    release(&win->lock);

    return 0;
}
