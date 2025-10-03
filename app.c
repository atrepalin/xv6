#include "types.h"
#include "stat.h"
#include "user.h"
#include "msg.h"

#define printf(...) printf(1, __VA_ARGS__)

int main() {
    int pid = fork();

    struct msg msg;

    if(pid) {
        create_window(100, 100, 200, 200);
        fill_window(0, 255, 0);
    } else {
        create_window(0, 0, 200, 200);
        fill_window(255, 0, 0);
    }

    while(get_msg(&msg, 1)) {
        if(IS_MOUSE(msg.type)) {
            printf("%s: %d %d %d\n", pid ? "parent" : "child", 
                msg.type, msg.mouse.x, msg.mouse.y);
        }

        if(IS_ESC(msg)) {
            exit();
        }
    }
}