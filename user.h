struct stat;
struct rtcdate;
struct msg;

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int*);
int write(int, const void*, int);
int read(int, void*, int);
int close(int);
int kill(int);
int exec(char*, char**);
int open(const char*, int);
int mknod(const char*, short, short);
int unlink(const char*);
int fstat(int fd, struct stat*);
int link(const char*, const char*);
int mkdir(const char*);
int chdir(const char*);
int dup(int);
int getpid(void);
char* sbrk(int);
int sleep(int);
int uptime(void);
int poweroff(void);

// ulib.c
int stat(const char*, struct stat*);
char* strcpy(char*, const char*);
void *memmove(void*, const void*, int);
char* strchr(const char*, char c);
int strcmp(const char*, const char*);
void printf(int, const char*, ...);
void scanf(int fd, char *fmt, ...);
char* gets(char*, int max);
uint strlen(const char*);
void* memset(void*, int, uint);
void* malloc(uint);
void free(void*);
int atoi(const char*);

int destroy_window(void);
int invalidate_window(int, int, int, int);
int fill_window(uchar, uchar, uchar);
int fill_rect(int, int, int, int, uchar, uchar, uchar);
int draw_line(int, int, int, int, uchar, uchar, uchar);
int draw_text(int, int, uchar, uchar, uchar, const char*);
int bring_to_front(void);
int move_window(int, int);
int minimize_window(void);
int get_msg(struct msg*, int);
int capture_window(void);
int release_window(void);