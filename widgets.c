#include "widgets.h"
#include "uwindow.h"
#include "user.h"
#include "param.h"

void add_widget(struct user_window *win, struct widget *w)
{
    if (!win || !w) return;
    w->win = win;
    w->next = 0;
    if (!win->first)
        win->first = w;
    else {
        struct widget *cur = win->first;
        while (cur->next) cur = cur->next;
        cur->next = w;
    }

    struct msg msg = { .type = M_DRAW };
    w->handler(&msg, win, w);
}

void remove_widget(struct widget *w) {
    struct user_window *win = w->win;

    if (!win || !win->first || !w) return;

    struct widget *cur = win->first, *prev = 0;
    while (cur) {
        if (cur == w) {
            if (prev) prev->next = cur->next;
            else win->first = cur->next;
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

void focus_widget(struct widget *w) {
    if (w->focusable)
        w->win->focused = w;
}

void set_text(struct widget *w, const char *text) {
    strcpy(w->text, text);
    struct msg msg = { .type = M_DRAW };
    w->handler(&msg, w->win, w);
}

char* slice_text(struct widget *w) {
    int max_chars = (w->w - 8) / CHARACTER_WIDTH;

    static char buf[64];
    int len = strlen(w->text);
    if (len > max_chars) len = max_chars;
    memmove(buf, w->text, len);
    buf[len] = '\0';

    return buf;
}

int label_handler(struct msg *msg, struct user_window *win, struct widget *self) {
    if (msg->type == M_DRAW) {
        fill_rect(self->x, self->y, self->w, self->h, 60, 60, 60);
        draw_text(self->x + 4, self->y + self->h / 2 - CHARACTER_HEIGHT / 2, 255, 255, 255, slice_text(self));
        return 1;
    }

    return 0;
}

int button_handler(struct msg *msg, struct user_window *win, struct widget *self) {
    if (msg->type == M_DRAW) {
        fill_rect(self->x, self->y, self->w, self->h, 80, 80, 180);
        draw_text_centered(self->x + self->w / 2, self->y + self->h / 2, 255, 255, 255, slice_text(self));
        return 1;
    }

    if (msg->type == M_MOUSE_LEFT_CLICK) {
        int mx = msg->mouse.x, my = msg->mouse.y;

        if (mx >= self->x && mx <= self->x + self->w && my >= self->y && my <= self->y + self->h) {
            if(self->focusable)
                win->focused = self;

            button_click_cb cb = (button_click_cb)self->userdata;
            if (cb) 
                cb(self);

            return 1;
        }
    }

    return 0;
}

int textbox_handler(struct msg *msg, struct user_window *win, struct widget *self) {
    int max_chars = (self->w - 8) / CHARACTER_WIDTH;

    if (msg->type == M_DRAW) {
draw:
        fill_rect(self->x, self->y, self->w, self->h, 60, 60, 60);        
        draw_text(self->x + 4, self->y + self->h / 2 - CHARACTER_HEIGHT / 2, 255, 255, 255, slice_text(self));
        return 1;
    }

    if (msg->type == M_MOUSE_LEFT_CLICK && self->focusable) {
        int mx = msg->mouse.x, my = msg->mouse.y;
        if (mx >= self->x && mx <= self->x + self->w && my >= self->y && my <= self->y + self->h) {
            win->focused = self;
        }
    }

    if (msg->type == M_KEY_DOWN && win->focused == self) {
        uint len = strlen(self->text);

        if (msg->key.charcode == 8 && len > 0) {
            self->text[len - 1] = '\0';
        }
        else if (msg->key.charcode >= 32 && len < sizeof(self->text) - 1) {
            self->text[len++] = msg->key.charcode;
            self->text[len] = '\0';
        }

        if (strlen(self->text) > max_chars) {
            self->text[max_chars] = '\0';
        }

        textbox_change_cb cb = (textbox_change_cb)self->userdata;

        if (cb) 
            cb(self, self->text);

        goto draw;
    }

    return 0;
}

struct widget *create_label(int x, int y, int w, int h, const char *text) {
    struct widget *lbl = malloc(sizeof(struct widget));
    lbl->x = x;
    lbl->y = y;
    lbl->w = w;
    lbl->h = h;
    strcpy(lbl->text, text);
    lbl->handler = label_handler;
    return lbl;
}

struct widget *create_button(int x, int y, int w, int h, const char *text, button_click_cb cb) {
    struct widget *btn = malloc(sizeof(struct widget));
    btn->x = x;
    btn->y = y;
    btn->w = w;
    btn->h = h;
    btn->focusable = 1;
    strcpy(btn->text, text);
    btn->handler = button_handler;
    btn->userdata = cb;
    return btn;
}

struct widget *create_textbox(int x, int y, int w, int h, const char *text, textbox_change_cb cb) {
    struct widget *tb = malloc(sizeof(struct widget));
    tb->x = x;
    tb->y = y;
    tb->w = w;
    tb->h = h;
    tb->focusable = 1;
    strcpy(tb->text, text);
    tb->handler = textbox_handler;
    tb->userdata = cb;
    return tb;
}