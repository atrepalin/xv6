#include "window.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "stat.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "elf.h"

extern int screen_w, screen_h;

struct window *progman_win = 0;

#define BAR_HEIGHT 30

#define BACKGROUND    PACK(255, 255, 255)
#define BAR           100, 100, 255
#define FOREGROUND    255, 255, 255
#define DT_FOREGROUND 0,   0,   0
#define TASK          70,  70,  200
#define ICON          180, 180, 255

#define MAX_TASKS 32

static struct window *task_list[MAX_TASKS];
static int task_count = 0;

#define ICON_W 48
#define ICON_H 48
#define ICON_PADDING_X 20
#define ICON_PADDING_Y 30
#define ICON_START_X 20
#define ICON_START_Y 20

struct desktop_icon {
    char name[32];
    int x, y;
};

#define MAX_ICONS 64

static struct desktop_icon icons[MAX_ICONS];
static int icon_count = 0;

#define MAX_LAUNCH_NAME 32

struct {
  struct spinlock lock;
  char pending[MAX_LAUNCH_NAME];
  int has_task;
} launchq;

static int is_executable_inode(struct inode *ip) {
    struct elfhdr elf;

    if (readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
        return 0;

    return elf.magic == ELF_MAGIC;
}

const char *ignorable[] = {
    "init"
};

void init_desktop(void)
{
    struct inode *dp;
    struct dirent de;

    int icon_x = ICON_START_X;
    int icon_y = ICON_START_Y;

    dp = namei("/");
    if (dp == 0) return;

    ilock(dp);
    int offset = 0;
    while (readi(dp, (char*)&de, offset, sizeof(de)) == sizeof(de)) {
        offset += sizeof(de);

        if (de.inum == 0) continue;

        if (de.inum == dp->inum) {
            continue;
        }

        struct inode *ip = iget(dp->dev, de.inum);
        ilock(ip);

        if (ip->type == T_FILE && is_executable_inode(ip)) {
            for (int i = 0; i < sizeof(ignorable) / sizeof(ignorable[0]); i++) {
                if (strncmp(de.name, ignorable[i], strlen(ignorable[i])) == 0) {
                    goto next;
                }
            }

            if (icon_count < MAX_ICONS) {
                struct desktop_icon *ic = &icons[icon_count++];
                safestrcpy(ic->name, de.name, sizeof(ic->name));
                ic->x = icon_x;
                ic->y = icon_y;
            }

            icon_x += ICON_W + ICON_PADDING_X;
            if (icon_x + ICON_W > screen_w - ICON_PADDING_X) {
                icon_x = ICON_START_X;
                icon_y += ICON_H + ICON_PADDING_Y;
            }

            if (icon_y + ICON_H > screen_h - CHARACTER_HEIGHT - BAR_HEIGHT - 16) {
                break;
            }
        }

next:
        iunlockput(ip);
    }

    iunlockput(dp);
}

void draw_desktop(void) {
    for (int i = 0; i < icon_count; i++) {
        struct desktop_icon *ic = &icons[i];

        fill_rect(ic->x, ic->y, ICON_W, ICON_H, ICON, progman_win);

        char namebuf[7];
        int len = strlen(ic->name);
        if (len > 6) len = 6;
        memmove(namebuf, ic->name, len);
        namebuf[len] = '\0';

        int text_w = len * CHARACTER_WIDTH;
        int text_x = ic->x + (ICON_W - text_w) / 2;
        int text_y = ic->y + ICON_H + 2;

        draw_text(text_x, text_y, DT_FOREGROUND, namebuf, progman_win);
    }

    invalidate(0, 0, screen_w, screen_h - 30);
}

void draw_taskbar(void) {
    fill_rect(0, screen_h - BAR_HEIGHT, screen_w, BAR_HEIGHT, BAR, progman_win);

    int x = 5;
    for (int i = 0; i < task_count; i++) {
        struct window *w = task_list[i];

        char namebuf[10];
        int len = strlen(w->title);
        if (len > 9) len = 9;
        memmove(namebuf, w->title, len);
        namebuf[len] = '\0';

        fill_rect(x - 2, screen_h - BAR_HEIGHT + 3, 90, BAR_HEIGHT - 6, TASK, progman_win);
        draw_text(x, screen_h - BAR_HEIGHT + CHARACTER_HEIGHT / 2 - 2, FOREGROUND, namebuf, progman_win);

        x += 100;
    }

    invalidate(0, screen_h - BAR_HEIGHT, screen_w, BAR_HEIGHT);
}

void init_launchq(void) {
    initlock(&launchq.lock, "launchq");
    launchq.has_task = 0;
}

void init_progman(void) {
    progman_win = kmalloc(sizeof(struct window));

    initlock(&progman_win->lock, "window");
    progman_win->x = 0;
    progman_win->y = 0;
    progman_win->w = screen_w;
    progman_win->h = screen_h;
    progman_win->backbuf = kmalloc(screen_w * screen_h * sizeof(struct rgb));
    progman_win->owner = 0;

    memset_fast_long(progman_win->backbuf, BACKGROUND, screen_w * screen_h);

    add_window(progman_win);

    invalidate(0, 0, screen_w, screen_h);

    init_desktop();
    draw_desktop();

    init_launchq();

    draw_taskbar();
}

void onaddwindow(struct window *win) {
    if (win == progman_win) return;

    if (task_count >= MAX_TASKS)
        return;

    task_list[task_count++] = win;
    draw_taskbar();
}

void onremovewindow(struct window *win) {
    for (int i = 0; i < task_count; i++) {
        if (task_list[i] == win) {  
            memmove(&task_list[i], &task_list[i + 1], (task_count - i - 1) * sizeof(struct window *));
            task_count--;
            break;
        }
    }

    draw_taskbar();
}

struct proc *init_proc;

extern void wait_for_msg(struct proc *p);
extern void wakeup_on_msg(struct proc *p);

void set_launch_request(const char *name) {
    acquire(&launchq.lock);
    safestrcpy(launchq.pending, name, sizeof(launchq.pending));
    launchq.has_task = 1;
    release(&launchq.lock);

    wakeup_on_msg(init_proc);
}

void onmsg(struct msg *msg) {
    if (msg->type != M_MOUSE_LEFT_CLICK || msg->mouse.id != 0)
        return;
        
    if (msg->mouse.y >= screen_h - BAR_HEIGHT) {
        int x = msg->mouse.x;
        int index = x / 100;

        if (index < task_count && (x % 100) < 90) {
            struct window *target = task_list[index];

            acquire(&target->lock);

            bring_to_front(target);
            invalidate(target->x, target->y, target->w, target->h);

            release(&target->lock);
        }
    } else {
        for (int i = 0; i < icon_count; i++) {
            struct desktop_icon *ic = &icons[i];
            if (msg->mouse.x >= ic->x && msg->mouse.x < ic->x + ICON_W &&
                msg->mouse.y >= ic->y && msg->mouse.y < ic->y + ICON_H) {

                set_launch_request(ic->name);
                return;
            }
        }
    }
}

int
sys_init_progman(void) {
    init_progman();

    init_proc = myproc();
    return 0;
}

int 
sys_get_launch(void) {
    char *buf;
    if (argptr(0, &buf, MAX_LAUNCH_NAME) < 0) return 0;

    acquire(&launchq.lock);
    if (!launchq.has_task) {
        release(&launchq.lock);
        wait_for_msg(init_proc);
        acquire(&launchq.lock);
    }

    safestrcpy(buf, launchq.pending, MAX_LAUNCH_NAME);
    launchq.has_task = 0;
    release(&launchq.lock);

    return 1;
}