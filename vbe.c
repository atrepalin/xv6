#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "vbe.h"

uchar *framebuffer = 0;
int screen_w, screen_h;

static void
vbe_write_reg(ushort index, ushort value) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}

void vbe_init(int w, int h) {
    screen_w = w;
    screen_h = h;

    if(framebuffer) return;
    
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write_reg(VBE_DISPI_INDEX_XRES, screen_w);
    vbe_write_reg(VBE_DISPI_INDEX_YRES, screen_h);
    vbe_write_reg(VBE_DISPI_INDEX_BPP, 32);
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    framebuffer = (uchar*)FB_VA;

    return;
}