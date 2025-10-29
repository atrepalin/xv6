// init: The initial user-level program

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

char *argv[] = { "sh", 0 };

extern int init_progman(void);
extern int get_launch(char *name);

int
main(void)
{
  int pid, ppid, wpid;

  if(open("console", O_RDWR) < 0){
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }
  dup(0);  // stdout
  dup(0);  // stderr

  for(;;){
    // printf(1, "init: starting sh\n");
    // pid = fork();
    // if(pid < 0){
    //   printf(1, "init: fork failed\n");
    //   exit();
    // }
    // if(pid == 0){
    //   exec("sh", argv);
    //   printf(1, "init: exec sh failed\n");
    //   exit();
    // }

    // ppid = fork();
    // if (ppid == 0) {
      init_progman();

      char launch_name[32];

      while(get_launch(launch_name)) {
        printf(1, "program manager: launching %s\n", launch_name);
        int pid = fork();

        if(pid == 0){
          char *argv[] = { launch_name, 0 };
          exec(launch_name, argv);
          printf(1, "program manager: exec %s failed\n", launch_name);
          exit();
        }
      }
    // }

    // while((wpid=wait()) >= 0 && (wpid != pid || wpid != ppid))
    //   printf(1, "zombie!\n");
  }
}
