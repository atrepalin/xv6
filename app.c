#include "types.h"
#include "stat.h"
#include "user.h"

#define abs(x) ((x) < 0 ? -(x) : (x))
#define COLOR(r, g, b) (((r) << 16) | ((g) << 8) | (b))

int main() {
    vbe_draw_triangle(100, 100, 150, 50, 200, 150, COLOR(255, 0, 0));

    vbe_draw_circle(350, 250, 50, COLOR(0, 255, 0));

    vbe_draw_rect(500, 400, 700, 500, COLOR(0, 0, 255));

    vbe_draw_line(100, 150, 200, 400, COLOR(255, 255, 0));

    int w = 128, h = 128;
    int *sprite = malloc(w * h * sizeof(int));

    for(int i = 0; i < w; i++) {
        for(int j = 0; j < h; j++) {
            int idx = i * w + j;

            int red   = 255 - (j * 255) / (w - 1);
            int green = (j * 255) / (w - 1);
            int blue  = (i * 255) / (h - 1);                     

            sprite[idx] = COLOR(red, green, blue);
        }
    }

    vbe_draw_sprite(500, 100, w, h, sprite);

    char c;
    while(read(0, &c, 1) == 1) {
        if(c == '\n') break;
    }

    vbe_clear();
    exit();
}