#include "window.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#include "cursor.h"

#define MAX_RECTS 128

struct rect {
    int x, y, w, h;
};

struct rect_list {
    struct rect rects[MAX_RECTS];
    int count;
} invalidated;

struct window *head = 0, *tail = 0;

struct spinlock vram_lock;

extern uchar *framebuffer;
uchar *backbuf;
extern int screen_w, screen_h;

extern int mouse_x, mouse_y;

extern struct window *progman_win;

void initcomposer(void) {
    initlock(&vram_lock, "vram");

    backbuf = kmalloc(screen_w * screen_h * sizeof(struct rgb));

    memset_fast_long(backbuf, 0, screen_w * screen_h);
}

void add_window(struct window *win) {
    acquire(&win->lock);

    win->next_z = 0;
    win->visible = 1;

    if (tail == 0) {
        head = tail = win;
    } else {
        tail->next_z = win;
        tail = win;
    }

    release(&win->lock);

    onaddwindow(win);
}

void destroy_window(struct window *win) {
    if (!win) return;

    onremovewindow(win);

    acquire(&win->lock);

    if (head == win && tail == win) {
        head = tail = 0;
    } else if (head == win) {
        head = win->next_z;
    } else {
        struct window *prev = head;
        while (prev && prev->next_z != win) {
            prev = prev->next_z;
        }

        if (prev) {
            prev->next_z = win->next_z;
            if (tail == win) {
                tail = prev;
            }
        }
    }

    invalidate(win->x, win->y, win->w, win->h);

    release(&win->lock);

    if (win->backbuf)
        kmfree(win->backbuf);

    kmfree(win);
}

void bring_to_front(struct window *win) {
    if (win == tail) return;

    win->visible = 1;

    struct window *prev = 0, *cur = head;

    while (cur && cur != win) {
        prev = cur;
        cur = cur->next_z;
    }

    if (!cur) {
        tail->next_z = win;
        tail = win;
        return;
    }

    if (prev) prev->next_z = cur->next_z;
    else head = cur->next_z;

    tail->next_z = cur;
    tail = cur;
    cur->next_z = 0;
}

void hide_window(struct window *win) {
    struct window *prev = 0, *cur = head;

    while (cur && cur != win) {
        prev = cur;
        cur = cur->next_z;
    }

    if (!cur) return;

    if (prev) prev->next_z = cur->next_z;
    else head = cur->next_z;

    tail = prev->next_z ? prev->next_z : prev;

    cur->next_z = 0;
    cur->visible = 0;
}

int click_bring_to_front(void) {
    struct window *target = 0;

    for (struct window *win = head; win; win = win->next_z) {
        if (mouse_x >= win->x && mouse_x < win->x + win->w &&
            mouse_y >= win->y && mouse_y < win->y + win->h && 
            win->visible) {
            target = win;
        }
    }

    if (target && target != tail && target != progman_win) {
        acquire(&target->lock);

        bring_to_front(target);
        invalidate(target->x, target->y, target->w, target->h);

        release(&target->lock);

        return 1;
    }

    return 0;
}

void invalidate(int x, int y, int w, int h) {
    if (invalidated.count >= MAX_RECTS) return;
    struct rect *r = &invalidated.rects[invalidated.count++];

    r->x = x; 
    r->y = y; 
    r->w = w; 
    r->h = h;
}

void draw_cursor(void) {
    if (mouse_x >= 0 && mouse_y >= 0 && mouse_x < screen_w && mouse_y < screen_h) {
        ulong col;

        for (int i = 0, y = mouse_y; i < MOUSE_HEIGHT && y < screen_h; i++, y++) {
            for (int j = 0, x = mouse_x; j < MOUSE_WIDTH && x < screen_w; j++, x++ ) {
                if ((col = mouse_pointer[i][j])) {
                    int offset = (y * screen_w + x) * sizeof(struct rgb);

                    ulong *dst = (ulong*)(framebuffer + offset);

                    *dst = col;
                }
            }
        }
    }
}

void compose(void) {
    acquire(&vram_lock);

    for (int i = 0; i < invalidated.count; i++) {
        struct rect *r = &invalidated.rects[i];

        int x0 = r->x < 0 ? 0 : r->x;
        int y0 = r->y < 0 ? 0 : r->y;
        int x1 = (r->x + r->w > screen_w) ? screen_w : r->x + r->w;
        int y1 = (r->y + r->h > screen_h) ? screen_h : r->y + r->h;

        if (x0 >= x1 || y0 >= y1) continue;
        
        for (struct window *win = head; win; win = win->next_z) {
            if (!win->visible) continue;

            int wx0 = win->x;
            int wy0 = win->y;
            int wx1 = win->x + win->w;
            int wy1 = win->y + win->h;

            int rx0 = (x0 > wx0) ? x0 : wx0;
            int ry0 = (y0 > wy0) ? y0 : wy0;
            int rx1 = (x1 < wx1) ? x1 : wx1;
            int ry1 = (y1 < wy1) ? y1 : wy1;

            if (rx0 >= rx1 || ry0 >= ry1) continue;

            int row_bytes = (rx1 - rx0) * sizeof(struct rgb);

            acquire(&win->lock);

            for (int y = ry0; y < ry1; y++) {
                uchar *dst = backbuf + (y * screen_w + rx0) * sizeof(struct rgb);
                struct rgb *src = win->backbuf + (y - win->y) * win->w + (rx0 - win->x);

                memcpy_fast(dst, src, row_bytes);
            }

            release(&win->lock);
        }
    }

    invalidated.count = 0;

    memcpy_fast(framebuffer, backbuf, screen_w * screen_h * sizeof(struct rgb));
    
    draw_cursor();

    release(&vram_lock);
}