#define MOUSE_HEIGHT 16
#define MOUSE_WIDTH 10

#define PACK(r, g, b) ((r << 16) | (g << 8) | b)

#define D 16
#define L PACK(200, 200, 200)

unsigned long mouse_pointer[MOUSE_HEIGHT][MOUSE_WIDTH] = {
    {L, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {L, L, 0, 0, 0, 0, 0, 0, 0, 0},
    {L, D, L, 0, 0, 0, 0, 0, 0, 0},
    {L, D, D, L, 0, 0, 0, 0, 0, 0},
    {L, D, D, D, L, 0, 0, 0, 0, 0},
    {L, D, D, D, D, L, 0, 0, 0, 0},
    {L, D, D, D, D, D, L, 0, 0, 0},
    {L, D, D, D, D, D, D, L, 0, 0},
    {L, D, D, D, D, D, D, D, L, 0},
    {L, D, D, D, D, D, L, L, L, L},
    {L, D, D, L, D, D, L, 0, 0, 0},
    {L, D, L, L, D, D, D, L, 0, 0},
    {L, L, 0, 0, L, D, D, L, 0, 0},
    {L, 0, 0, 0, 0, L, D, D, L, 0},
    {0, 0, 0, 0, 0, L, D, D, L, 0},
    {0, 0, 0, 0, 0, 0, L, L, 0, 0}
};