struct stat;
struct rtcdate;

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

// vbe.c
int vbe_disable(void);
int vbe_clear(void);
int vbe_putpixel(int x, int y, int color);
int vbe_draw_triangle(int x1, int y1, int x2, int y2, int x3, int y3, int color);
int vbe_draw_circle(int cx, int cy, int r, int color);
int vbe_draw_rect(int x1, int y1, int x2, int y2, int color);
int vbe_draw_line(int x1, int y1, int x2, int y2, int color);
int vbe_draw_sprite(int x, int y, int w, int h, int *data);