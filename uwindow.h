#ifndef __UWINDOW_H__
#define __UWINDOW_H__
#include "types.h"
#include "msg.h"

struct widget;

struct user_window {
    int x, y, w, h;
    int dragging;
    const char* title;
    int drag_offset_x, drag_offset_y;
    uchar r, g, b;

    struct widget *first;
    struct widget *focused;
};

struct user_window create_window(int x, int y, int w, int h, 
    const char* title, uchar r, uchar g, uchar b);

int dispatch_msg(struct msg *msg, struct user_window *win);
void redraw_window(struct user_window *win);

int draw_text_centered(int x, int y, uchar r, uchar g, uchar b, const char *s);
#endif