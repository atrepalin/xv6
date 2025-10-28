#include "widgets.h"
#include "uwindow.h"
#include "user.h"
#include "param.h"

static void wrap_text(struct textblock_internal *tb, int max_width);

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

static void textblock_set_text(struct widget *w, const char *text) {
    struct textblock_internal *tb = &w->textblock;
    strcpy(tb->text, text);
    wrap_text(tb, w->w);
    tb->scroll = 0;
}

void set_text(struct widget *w, const char *text) {
    switch (w->type){
        case TEXTBLOCK:
            textblock_set_text(w, text);
            break;
        case LABEL:
        case TEXTBOX:
        case BUTTON:
            strcpy(w->has_text.text, text);
            break;
    } 
    
    struct msg msg = { .type = M_DRAW };
    w->handler(&msg, w->win, w);
}

char* slice_text(struct widget *w) {
    int max_chars = (w->w - 8) / CHARACTER_WIDTH;

    static char buf[64];
    int len = strlen(w->has_text.text);
    if (len > max_chars) len = max_chars;
    memmove(buf, w->has_text.text, len);
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
        uint len = strlen(self->has_text.text);

        if (msg->key.charcode == 8 && len > 0) {
            self->has_text.text[len - 1] = '\0';
        }
        else if (msg->key.charcode >= 32 && len < sizeof(self->has_text.text) - 1) {
            self->has_text.text[len++] = msg->key.charcode;
            self->has_text.text[len] = '\0';
        }

        if (strlen(self->has_text.text) > max_chars) {
            self->has_text.text[max_chars] = '\0';
        }

        textbox_change_cb cb = (textbox_change_cb)self->userdata;

        if (cb) 
            cb(self, self->has_text.text);

        goto draw;
    }

    return 0;
}

#define CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

int scrollbar_handler(struct msg *msg, struct user_window *win, struct widget *self) {
    float *value = &self->scrollbar.value;
    int vertical = self->h > self->w;

    static int drag_offset = 0;

    if (msg->type == M_DRAW) {
draw:
        fill_rect(self->x, self->y, self->w, self->h, 80, 80, 80);

        int bar_size = vertical ? self->h : self->w;
        int thumb_size = (bar_size / 5);
        int pos = (int)((bar_size - thumb_size) * (*value));

        int tx = self->x + (vertical ? 0 : pos);
        int ty = self->y + (vertical ? pos : 0);
        int tw = vertical ? self->w : thumb_size;
        int th = vertical ? thumb_size : self->h;

        fill_rect(tx, ty, tw, th, 180, 180, 180);
        draw_line(tx, ty, tx + tw - 1, ty, 0, 0, 0);
        draw_line(tx, ty + th - 1, tx + tw - 1, ty + th - 1, 0, 0, 0);

        return 1;
    }

    int mx = msg->mouse.x;
    int my = msg->mouse.y;
    int inside = (mx >= self->x && mx <= self->x + self->w &&
                  my >= self->y && my <= self->y + self->h);

    if (msg->type == M_MOUSE_DOWN && inside) {
        int bar_size = vertical ? self->h : self->w;
        int thumb_size = (bar_size / 5);
        int pos = (int)((bar_size - thumb_size) * (*value));

        int tx = self->x + (vertical ? 0 : pos);
        int ty = self->y + (vertical ? pos : 0);
        int tw = vertical ? self->w : thumb_size;
        int th = vertical ? thumb_size : self->h;

        if (mx >= tx && mx <= tx + tw && my >= ty && my <= ty + th) {
            self->scrollbar.dragging = 1;
            drag_offset = vertical ? (my - ty) : (mx - tx);
        }
    }

    if (msg->type == M_MOUSE_UP) {
        self->scrollbar.dragging = 0;
    }

    if (msg->type == M_MOUSE_MOVE && self->scrollbar.dragging) {
        int bar_size = vertical ? self->h : self->w;
        int thumb_size = (bar_size / 5);
        int pos = vertical ? (my - self->y - drag_offset) : (mx - self->x - drag_offset);
        *value = (float)pos / (float)(bar_size - thumb_size);
        *value = CLAMP(*value, 0.0f, 1.0f);

        scroll_change_cb cb = (scroll_change_cb)self->userdata;
        if (cb) 
            cb(self, *value);

        goto draw;
    }

    if (msg->type == M_MOUSE_SCROLL && inside) {
        float delta = (msg->mouse.scroll > 0) ? -0.05f : 0.05f;
        *value = CLAMP(*value + delta, 0.0f, 1.0f);

        scroll_change_cb cb = (scroll_change_cb)self->userdata;
        if (cb) 
            cb(self, *value);

        goto draw;
    }

    return 0;
}

void wrap_text(struct textblock_internal *tb, int max_width) {
    static char text[1024];

    strcpy(text, tb->text);

    tb->line_count = 0;

    char *s = text;
    while (*s && tb->line_count < MAX_LINES) {
        char *line_start = s;
        int width = 0;
        int last_space = -1;
        int i = 0;

        while (s[i]) {
            if (s[i] == '\n') {
                s[i] = '\0';
                tb->lines[tb->line_count++] = line_start;
                s = &s[i + 1];
                goto next_line;
            }

            width += CHARACTER_WIDTH;
            if (width > max_width - PADDING * 2 - 6) {
                if (last_space >= 0) {
                    s[last_space] = '\0';
                    tb->lines[tb->line_count++] = line_start;
                    s = &s[last_space + 1];
                } else {
                    memmove_safe(s + i + 1, s + i, strlen(s + i) + 1);
                    s[i] = '\0';
                    tb->lines[tb->line_count++] = line_start;
                    s = &s[i + 1];
                }
                
                goto next_line;
            }

            if (s[i] == ' ')
                last_space = i;
            i++;
        }

        tb->lines[tb->line_count++] = line_start;
        break;

    next_line:;
    }
}

int textblock_handler(struct msg *msg, struct user_window *win, struct widget *self) {
    struct textblock_internal *tb = &self->textblock;

    int visible_lines = (self->h - PADDING * 2) / CHARACTER_HEIGHT;
    if (visible_lines <= 0) visible_lines = 1;

    int mx = msg->mouse.x, my = msg->mouse.y;
    int inside = (mx >= self->x && mx <= self->x + self->w &&
                  my >= self->y && my <= self->y + self->h);
    int focused = win->focused == self;

    if (msg->type == M_DRAW) {
draw:
        fill_rect(self->x, self->y, self->w, self->h, 40, 40, 40);

        int total_lines = tb->line_count;
        if (total_lines == 0) return 1;

        int max_offset = total_lines > visible_lines ? total_lines - visible_lines : 0;
        int scroll_offset = (int)(tb->scroll * max_offset + 0.5f);

        int y = self->y + PADDING;
        for (int i = scroll_offset; i < tb->line_count && i < scroll_offset + visible_lines; i++) {
            draw_text(self->x + PADDING, y, 255, 255, 255, tb->lines[i]);
            y += CHARACTER_HEIGHT;
        }

        if (total_lines > visible_lines) {
            int bar_h = (int)((float)visible_lines / (float)total_lines * self->h);
            if (bar_h < 10) bar_h = 10;
            int bar_y = self->y + (int)(tb->scroll * (self->h - bar_h));
            fill_rect(self->x + self->w - 6, self->y, 6, self->h, 25, 25, 25);
            fill_rect(self->x + self->w - 6, bar_y, 6, bar_h, 180, 180, 180);
        }

        return 1;
    }

    if (msg->type == M_MOUSE_SCROLL && inside && focused) {
        tb->scroll += (msg->mouse.scroll < 0 ? -0.05f : 0.05f);
        if (tb->scroll < 0) tb->scroll = 0;
        if (tb->scroll > 1) tb->scroll = 1;
        
        goto draw;
    }

    if (msg->type == M_MOUSE_LEFT_CLICK && inside) {
        if (self->focusable)
            win->focused = self;
            
        return 1;
    }

    if (msg->type == M_KEY_DOWN && focused) {
        int len = strlen(tb->text);

        if (msg->key.charcode == 8 && len > 0) {
            tb->text[len - 1] = '\0';
        } else if (msg->key.charcode == '\n') {
            if (len < MAX_TEXT - 2) {
                tb->text[len++] = '\n';
                tb->text[len] = '\0';
            }
        } else if (msg->key.charcode >= 32 && len < MAX_TEXT - 1) {
            tb->text[len++] = msg->key.charcode;
            tb->text[len] = '\0';
        }

        wrap_text(tb, self->w);
        textblock_change_cb cb = (textblock_change_cb)self->userdata;
        if (cb) 
            cb(self, tb->text);

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
    strcpy(lbl->has_text.text, text);
    lbl->handler = label_handler;
    lbl->type = LABEL;
    return lbl;
}

struct widget *create_button(int x, int y, int w, int h, const char *text, button_click_cb cb) {
    struct widget *btn = malloc(sizeof(struct widget));
    btn->x = x;
    btn->y = y;
    btn->w = w;
    btn->h = h;
    btn->focusable = 1;
    strcpy(btn->has_text.text, text);
    btn->handler = button_handler;
    btn->userdata = cb;
    btn->type = BUTTON;
    return btn;
}

struct widget *create_textbox(int x, int y, int w, int h, const char *text, textbox_change_cb cb) {
    struct widget *tb = malloc(sizeof(struct widget));
    tb->x = x;
    tb->y = y;
    tb->w = w;
    tb->h = h;
    tb->focusable = 1;
    strcpy(tb->has_text.text, text);
    tb->handler = textbox_handler;
    tb->userdata = cb;
    tb->type = TEXTBOX;
    return tb;
}

struct widget *create_scrollbar(int x, int y, int w, int h, scroll_change_cb cb) {
    struct widget *sb = malloc(sizeof(struct widget));
    sb->x = x;
    sb->y = y;
    sb->w = w;
    sb->h = h;
    sb->scrollbar.value = 0;
    sb->handler = scrollbar_handler;
    sb->userdata = (void*)cb;
    sb->type = SCROLLBAR;
    return sb;
}

struct widget *create_textblock(int x, int y, int w, int h, const char *text, textblock_change_cb cb) {
    struct widget *tb = malloc(sizeof(struct widget));
    tb->x = x;
    tb->y = y;
    tb->w = w;
    tb->h = h;
    tb->focusable = 1;
    tb->handler = textblock_handler;
    tb->userdata = cb;
    tb->type = TEXTBLOCK;
    strcpy(tb->textblock.text, text);
    wrap_text(&tb->textblock, w);
    return tb;
}