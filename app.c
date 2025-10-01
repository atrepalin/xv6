#include "types.h"
#include "stat.h"
#include "user.h"

void read_key() {
    char c;
    while(read(0, &c, 1) == 1) {
        if(c == '\n') break;
    }
}

int main() {
    int pid = fork();

    if(pid == 0) {
        create_window(0, 0, 200, 200);
        fill_window(255, 0, 0);

        while(1) {
            sleep(1000);
        }
    } else {
        create_window(100, 100, 200, 200);
        fill_window(0, 255, 0);

        read_key();

        bring_to_front();

        read_key();

        kill(pid);
    }

    exit();
}