#include "window.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "character.h"

int
sys_create_window(void)
{
    int x, y, w, h;
    char *title;

    if (argint(0, &x) < 0) return -1;
    if (argint(1, &y) < 0) return -1;
    if (argint(2, &w) < 0) return -1;
    if (argint(3, &h) < 0) return -1;
    if (argstr(4, &title) < 0) return -1;

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
    win->title = title;

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

    invalidate(win->x, win->y, win->w, win->h);

    return 0;
}

int
sys_destroy_window(void) {
    struct proc *p = myproc();
    if (!p->win) return -1;

    struct window *win = p->win;

    destroy_window(win);
    p->win = 0;

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

    acquire(&win->lock);
    invalidate(win->x + x, win->y + y, w, h);
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

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void 
fill_rect(int x, int y, int w, int h, int r, int g, int b, struct window *win) {
    acquire(&win->lock);

    if (x < 0) {
        w -= x;
        x = 0;
    }

    if (y < 0) {
        h -= y;
        y = 0;
    }

    if (x + w > win->w) {
        w = win->w - x;
    }

    if (y + h > win->h) {
        h = win->h - y;
    }
    
    for (int i = 0; i < h; i++) {
        memset_fast_long(win->backbuf + (win->w * (y + i) + x), PACK(r, g, b), w);
    }

    invalidate(win->x + x, win->y + y, w, h);
    release(&win->lock);
}

int
sys_fill_rect(void)
{
    int x, y, w, h, r, g, b;

    if (argint(0, &x) < 0) return -1;
    if (argint(1, &y) < 0) return -1;
    if (argint(2, &w) < 0) return -1;
    if (argint(3, &h) < 0) return -1;
    if (argint(4, &r) < 0) return -1;
    if (argint(5, &g) < 0) return -1;
    if (argint(6, &b) < 0) return -1;

    struct proc *p = myproc();
    if (!p->win) return -1;

    struct window *win = p->win;

    fill_rect(x, y, w, h, r, g, b, win);

    return 0;
}

void
put_pixel(struct window *win, int x, int y, struct rgb color) {
    if (x < 0 || x >= win->w || y < 0 || y >= win->h) return;

    win->backbuf[win->w * y + x] = color;
}

int
draw_char(struct window *win, int x, int y, char c, struct rgb color) {
    int ord = c - 0x20;

    if (ord < 0 || ord >= CHARACTER_NUMBER) {
        return x;
    }

    for (int i = 0; i < CHARACTER_HEIGHT; i++) {
        for (int j = 0; j < CHARACTER_WIDTH; j++) {
            if (character[ord][i][j]) {
                put_pixel(win, x + j, y + i, color);
            }
        }
    }

    return x + CHARACTER_WIDTH;
}

#define abs(x) ((x) < 0 ? -(x) : (x))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

int
sys_draw_line(void) {
    int x1, y1, x2, y2, r, g, b;

    if (argint(0, &x1) < 0) return -1;
    if (argint(1, &y1) < 0) return -1;
    if (argint(2, &x2) < 0) return -1;
    if (argint(3, &y2) < 0) return -1;
    if (argint(4, &r) < 0) return -1;
    if (argint(5, &g) < 0) return -1;
    if (argint(6, &b) < 0) return -1;

    struct rgb color = RGB(r, g, b);

    struct proc *p = myproc();
    if (!p->win) return -1;

    struct window *win = p->win;
    
    int min_x = min(x1, x2) - 1;
    int max_x = max(x1, x2) + 1;
    int min_y = min(y1, y2) - 1;
    int max_y = max(y1, y2) + 1;

    int dx = x2 - x1;
    int dy = y2 - y1;

    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    float xinc = dx / (float)steps;
    float yinc = dy / (float)steps;

    float x = x1;
    float y = y1;

    acquire(&win->lock);

    for (int i = 0; i <= steps; i++) {
        put_pixel(p->win, x, y, color);
        x += xinc;
        y += yinc;
    }

    invalidate(win->x + min_x, win->y + min_y, max_x - min_x, max_y - min_y);
    release(&win->lock);

    return 0;
}

void
draw_text(int x, int y, int r, int g, int b, const char *s, struct window *win) {
    int sx = x;

    struct rgb color = RGB(r, g, b);

    acquire(&win->lock);

    for (int i = 0; i < strlen(s); i++) {
        x = draw_char(win, x, y, s[i], color);
    }
    
    invalidate(win->x + sx, win->y + y, x - sx + CHARACTER_WIDTH, CHARACTER_HEIGHT);
    release(&win->lock);
}

int 
sys_draw_text(void) {
    int x, y, r, g, b;
    char *s;

    if (argint(0, &x) < 0) return -1;
    if (argint(1, &y) < 0) return -1;
    if (argint(2, &r) < 0) return -1;
    if (argint(3, &g) < 0) return -1;
    if (argint(4, &b) < 0) return -1;
    if (argstr(5, &s) < 0) return -1;

    struct proc *p = myproc();
    if (!p->win) return -1;

    struct window *win = p->win;

    draw_text(x, y, r, g, b, s, win);

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

int
sys_minimize_window(void) {
    struct proc *p = myproc();
    if (!p->win) return -1;

    struct window *win = p->win;

    acquire(&win->lock);
    hide_window(win);

    invalidate(win->x, win->y, win->w, win->h);
    release(&win->lock);

    return 0;
}