#include "uwindow.h"
#include "stat.h"
#include "user.h"
#include "character.h"

#define TITLE_HEIGHT 20
#define BTN_SIZE     16

#define BAR        100, 100, 255
#define ClOSE_BTN  255, 50,  50
#define FOREGROUND 255, 255, 255
#define BORDER     128, 128, 128

struct user_window create_window(int x, int y, int w, int h, 
    const char *title, uchar r, uchar g, uchar b) {
    struct user_window win = {
        .x = x,
        .y = y,
        .w = w,
        .h = h,
        .title = title
    };

    sys_create_window(win.x, win.y, win.w, win.h);
    fill_window(r, g, b);

    fill_rect(0, 0, win.w, TITLE_HEIGHT, BAR);
    draw_text_centered(win.w / 2, TITLE_HEIGHT / 2, FOREGROUND, win.title);
    
    fill_rect(win.w - BTN_SIZE - 2, 2, BTN_SIZE, BTN_SIZE, ClOSE_BTN);
    draw_text_centered(win.w - BTN_SIZE / 2 - 3, TITLE_HEIGHT / 2, FOREGROUND, "X");
    
    draw_line(0, 0, win.w, 0, BORDER);
    draw_line(0, 0, 0, win.h, BORDER);
    draw_line(win.w - 1, 0, win.w - 1, win.h, BORDER);
    draw_line(0, win.h - 1, win.w, win.h - 1, BORDER);
    draw_line(0, TITLE_HEIGHT, win.w, TITLE_HEIGHT, BORDER);

    return win;
}

int drag_window(struct msg *msg, struct user_window *win) {
    if (msg->type == M_MOUSE_DOWN && msg->mouse.button == BTN_LEFT) {
        if (msg->mouse.y < TITLE_HEIGHT) {
            if (msg->mouse.x > win->w - BTN_SIZE - 2 &&
                msg->mouse.x < win->w - 2 && !win->dragging) {

                destroy_window();
                wait();
                exit();
            } else {
                win->dragging = 1;
                win->drag_offset_x = msg->mouse.x;
                win->drag_offset_y = msg->mouse.y;
                
                capture_window();
            }

            return 1;
        }
    }

    if (msg->type == M_MOUSE_UP && win->dragging) {
        win->dragging = 0;
        release_window();

        return 1;
    }

    if (msg->type == M_MOUSE_MOVE && win->dragging) {
        int dx = msg->mouse.x - win->drag_offset_x;
        int dy = msg->mouse.y - win->drag_offset_y;
        win->x += dx;
        win->y += dy;

        move_window(win->x, win->y);

        return 1;
    }

    return 0;
}

typedef int (*msg_handler)(struct msg *msg, struct user_window *win);

const msg_handler handlers[] = {
    drag_window
};
    
int dispatch_msg(struct msg *msg, struct user_window *win) {
    for (int i = 0; i < sizeof(handlers) / sizeof(msg_handler); i++) {
        if (handlers[i](msg, win)) return 1;
    }

    return 0;
}

int draw_text_centered(int x, int y, uchar r, uchar g, uchar b, const char *s) {
    return draw_text(x - strlen(s) * CHARACTER_WIDTH / 2, y - CHARACTER_HEIGHT / 2, r, g, b, s);
}