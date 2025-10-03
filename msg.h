#ifndef __MSG_H__
#define __MSG_H__

#define M_NONE 0

#define M_KEY_UP 1
#define M_KEY_DOWN 2

#define M_MOUSE_MOVE 3
#define M_MOUSE_DOWN 4
#define M_MOUSE_UP 5
#define M_MOUSE_LEFT_CLICK 6
#define M_MOUSE_RIGHT_CLICK 7

#define M_CUSTOM 8

#define IS_MOUSE(type) type > M_KEY_DOWN && type < M_CUSTOM
#define IS_KEY(type) type <= M_KEY_DOWN && type > M_NONE

#define IS_ESC(msg) msg.type == M_KEY_DOWN && msg.key.charcode == 27

#define BTN_LEFT 1
#define BTN_RIGHT 2

struct msg {
    int type;
    union {
        struct {
            int charcode;
            int pressed;
        } key;

        struct {
            int x, y;
            int button;
        } mouse;

        struct {
            int code;
            int arg1, arg2;
        } custom;
    };
};

void send_msg(struct msg *);

#endif