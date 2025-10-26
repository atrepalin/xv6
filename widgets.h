#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include "types.h"
#include "msg.h"

struct user_window;

typedef int (*widget_msg_handler)(struct msg *msg, struct user_window *win, struct widget *self);

struct widget {
    int x, y, w, h;
    char text[64];
    char focusable;
    widget_msg_handler handler;
    void *userdata;
    struct widget *next;
    struct user_window *win;
};

void add_widget(struct user_window *win, struct widget *w);
void remove_widget(struct widget *w);

void focus_widget(struct widget *w);
void set_text(struct widget *w, const char *text);

typedef void (*button_click_cb)(struct widget *btn);
typedef void (*textbox_change_cb)(struct widget *tb, const char *text);

struct widget *create_label(int x, int y, int w, int h, const char *text);
struct widget *create_button(int x, int y, int w, int h, const char *text, button_click_cb cb);
struct widget *create_textbox(int x, int y, int w, int h, const char *text, textbox_change_cb cb);
#endif