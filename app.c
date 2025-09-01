#include "types.h"
#include "stat.h"
#include "user.h"

int area(int x1, int y1, int x2, int y2, int x3, int y3) {
    int a = x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2);
    return a < 0 ? -a : a;
}

int main() {
    vbe_init();

    int x1 = 100, y1 = 100;
    int x2 = 150, y2 = 50;
    int x3 = 200, y3 = 150;

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
                vbe_putpixel(x, y, 0xFF0000);
            }
        }
    }

    int cx = 350;
    int cy = 250;
    int r = 50;

    for(int x = cx - r; x <= cx + r; x++) {
        for(int y = cy - r; y <= cy + r; y++) {
            int dx = x - cx;
            int dy = y - cy;
            if(dx * dx + dy * dy <= r * r) {
                vbe_putpixel(x, y, 0x00FF00);
            }
        }
    }

    for(int i = 500; i < 700; i++) { 
        for(int j = 400; j < 500; j++) {
            vbe_putpixel(i, j, 0x0000FF); 
        } 
    }

    char c;
    while(read(0, &c, 1) == 1) {
        if(c == '\n') break;
    }

    vbe_disable();
    exit();
}