#include "types.h"
#include "stat.h"
#include "user.h"
#include "uwindow.h"

#define printf(...) printf(1, __VA_ARGS__)

int main() {
    struct msg msg;
    struct user_window win;

    int pid = fork();
    
    if(pid == 0) {
        win = create_window(0, 0, 200, 200, "child window", 0, 0, 255);

        draw_text_centered(100, 100, 255, 255, 255, "Hello Everyone!");
    } else {
        win = create_window(100, 100, 200, 200, "parent window", 0, 255, 0);

        draw_text_centered(100, 100, 0, 0, 0, "Hello World!");
    }

    while(get_msg(&msg, 1)) {
        if(!dispatch_msg(&msg, &win)) {
            if(IS_MOUSE(msg.type)) {
                printf("%s: %d %d %d\n", pid ? "parent" : "child", 
                    msg.type, msg.mouse.x, msg.mouse.y);
            }
        }
    }
}