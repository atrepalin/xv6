#ifndef __WINDOW_H__
#define __WINDOW_H__
#include "types.h"
#include "spinlock.h"

struct rgb {
    uchar b;
    uchar g;
    uchar r;
    uchar padding;
} __attribute__((packed));

#define RGB(r, g, b) ((struct rgb){b, g, r, 0})
#define PACKED(rgb) ((rgb.r << 16) | (rgb.g << 8) | rgb.b)
#define PACK(r, g, b) ((r << 16) | (g << 8) | b)

struct window {
    int x, y;
    int w, h;              
    struct rgb *backbuf;
    struct window *next_z;
    int visible;
    struct proc *owner;
    struct spinlock lock;
};

#endif