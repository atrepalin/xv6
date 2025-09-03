#include "types.h"
#include "user.h"

#define printf(...) printf(1, __VA_ARGS__)
#define scanf(...) scanf(0, __VA_ARGS__)

void exec_pipe(int fd) {
    int num;

    read(fd, &num, sizeof(num));
    printf("prime %d\n", num);

    int p[2];

    pipe(p);

    int tmp = 0;

    while(read(fd, &tmp, sizeof(tmp)) > 0) {
        if(tmp % num != 0) {
            write(p[1], &tmp, sizeof(tmp));
        }
    }

    if(!tmp) {
        close(p[1]);
        close(p[0]);
        close(fd);
        return;
    }

    if(!fork()) {
        close(p[1]);
        close(fd);
        exec_pipe(p[0]);
        close(p[0]);
    }
    else {
        close(p[0]);
        close(p[1]);
        close(fd);
        wait();
    }
}

int main()
{
    int p[2];
    pipe(p);
    
    int max;

    scanf("%d", &max);

    for(int i = 2; i <= max; i++)
    {
        write(p[1], &i, sizeof(i));
    }

    close(p[1]);
    exec_pipe(p[0]);
    close(p[0]);

    exit();
}