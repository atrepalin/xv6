#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "vbe.h"

static uint *lfb = 0;
static int width = 800;
static int height = 600;

static void
vbe_write_reg(ushort index, ushort value) {
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA, value);
}

int sys_vbe_init(void) {
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write_reg(VBE_DISPI_INDEX_XRES, width);
    vbe_write_reg(VBE_DISPI_INDEX_YRES, height);
    vbe_write_reg(VBE_DISPI_INDEX_BPP, 32);
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    lfb = (volatile uint*)FB_VA;

    return 0;
}

int sys_vbe_disable(void) {
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);

    lfb = 0;

    return 0;
}

int sys_vbe_putpixel(void) {
    int x, y, color;
    if(argint(0, &x) < 0 || argint(1, &y) < 0 || argint(2, &color) < 0)
        return -1;

    if(!lfb) return -1;

    if(x < 0) { x = 0; }
    if(y < 0) { y = 0; }
    if(x >= width || y >= height) return 0;

    lfb[y * width + x] = color;

    return 0;
}