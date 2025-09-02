#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "vbe.h"

static uint *lfb = 0;
static int width;
static int height;

static void
vbe_write_reg(ushort index, ushort value) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}

int sys_vbe_init(void) {
    if(argint(0, &width) < 0 || argint(1, &height) < 0) 
        return -1;

    if(lfb) return -1;
    
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write_reg(VBE_DISPI_INDEX_XRES, width);
    vbe_write_reg(VBE_DISPI_INDEX_YRES, height);
    vbe_write_reg(VBE_DISPI_INDEX_BPP, 32);
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    lfb = (uint*)FB_VA;

    return 0;
}

int sys_vbe_disable(void) {
    if(!lfb) return -1;

    memset(lfb, 0, FB_SIZE);

    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);

    lfb = 0;

    return 0;
}

int sys_vbe_clear(void) {
    if(!lfb) return -1;

    memset(lfb, 0, FB_SIZE);

    return 0;
}

int putpixel(int x, int y, int color) {
    if(!lfb) return -1;
    
    if(x < 0 || y < 0 || x >= width || y >= height) return 0;

    lfb[y * width + x] = color;

    return 0;
}

int sys_vbe_putpixel(void) {
    int x, y, color;

    if(argint(0, &x) < 0 || argint(1, &y) < 0 || argint(2, &color) < 0)
        return -1;

    if(!lfb) return -1;

    return putpixel(x, y, color);
}

#define abs(x) ((x) < 0 ? -(x) : (x))

int area(int x1, int y1, int x2, int y2, int x3, int y3) {
    int a = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);
    return abs(a);
}

int sys_vbe_draw_triangle(void) {
    int x1, y1, x2, y2, x3, y3, color;

    if(argint(0, &x1) < 0 || argint(1, &y1) < 0 
    || argint(2, &x2) < 0 || argint(3, &y2) < 0 
    || argint(4, &x3) < 0 || argint(5, &y3) < 0 
    || argint(6, &color) < 0)
        return -1;

    if(!lfb) return -1;

    int minX = x1 < x2 ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
    int maxX = x1 > x2 ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
    int minY = y1 < y2 ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
    int maxY = y1 > y2 ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

    for(int x = minX; x <= maxX; x++) {
        for(int y = minY; y <= maxY; y++) {
            int A = area(x1, y1, x2, y2, x3, y3);
            int A1 = area(x, y, x2, y2, x3, y3);
            int A2 = area(x1, y1, x, y, x3, y3);
            int A3 = area(x1, y1, x2, y2, x, y);
            if(A == A1 + A2 + A3) {
                putpixel(x, y, color);
            }
        }
    }

    return 0;
}

int sys_vbe_draw_circle(void) {
    int cx, cy, r, color;

    if(argint(0, &cx) < 0 || argint(1, &cy) < 0 
    || argint(2, &r) < 0 || argint(3, &color) < 0)
        return -1;

    if(!lfb) return -1;

    for(int x = cx - r; x <= cx + r; x++) {
        for(int y = cy - r; y <= cy + r; y++) {
            int dx = x - cx;
            int dy = y - cy;
            if(dx * dx + dy * dy <= r * r) {
                putpixel(x, y, color);
            }
        }
    }

    return 0;
}

int sys_vbe_draw_rect(void) {
    int x1, y1, x2, y2, color;

    if(argint(0, &x1) < 0 || argint(1, &y1) < 0 
    || argint(2, &x2) < 0 || argint(3, &y2) < 0 
    || argint(4, &color) < 0)
        return -1;

    if(!lfb) return -1;

    for(int x = x1; x <= x2; x++) {
        for(int y = y1; y <= y2; y++) {
            putpixel(x, y, color);
        }
    }

    return 0;
}

int sys_vbe_draw_line(void) {
    int x1, y1, x2, y2, color;

    if(argint(0, &x1) < 0 || argint(1, &y1) < 0 
    || argint(2, &x2) < 0 || argint(3, &y2) < 0 
    || argint(4, &color) < 0)
        return -1;

    if(!lfb) return -1;

    int dx = x2 - x1;
    int dy = y2 - y1;

    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);

    float xinc = dx / (float)steps;
    float yinc = dy / (float)steps;

    float x = x1;
    float y = y1;

    for (int i = 0; i <= steps; i++) {
        putpixel((int)x, (int)y, color);
        x += xinc;
        y += yinc;
    }

    return 0;
}

int sys_vbe_draw_sprite(void) {
    int x, y, w, h;
    char *ptr;

    if(argint(0, &x) < 0 || argint(1, &y) < 0 
    || argint(2, &w) < 0 || argint(3, &h) < 0 
    || argptr(4, &ptr, w * h * sizeof(int)) < 0)
        return -1;

    if(!lfb) return -1;

    int *pixels = (int*)ptr;

    for(int i = 0; i < h; i++) {
        for(int j = 0; j < w; j++) {
            putpixel(x + j, y + i, pixels[i * w + j]);
        }
    }

    return 0;
}