#include "types.h"
#include "stat.h"
#include "user.h"
#include "msg.h"

int main() {
    int pid = fork();

    if(pid == 0) {
        create_window(0, 0, 200, 200);
        fill_window(255, 0, 0);

        struct msg* msg = malloc(sizeof(struct msg));

        while(get_msg(msg, 1)) {
            if(IS_MOUSE(msg->type))
                printf(1, "child: %d %d %d\n", msg->type, msg->mouse.x, msg->mouse.y);

            if(IS_ESC(msg)) {
                exit();
            }
        }
    } else {
        create_window(100, 100, 200, 200);
        fill_window(0, 255, 0);

        struct msg* msg = malloc(sizeof(struct msg));

        while(get_msg(msg, 1)) {
            if(IS_MOUSE(msg->type))
                printf(1, "parent: %d %d %d\n", msg->type, msg->mouse.x, msg->mouse.y);

            if(IS_ESC(msg)) {
                exit();
            }
        }
    }

    exit();
}