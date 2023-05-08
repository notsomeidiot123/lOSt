#include "../libs/math.h"

typedef struct {
    short mode;
    char text: 1;
    char colors: 4;
    int xres;
    int yres;
    //0=1, 1=16, 2=256, and so on
}vga_descriptor_t;

unsigned short *vga_buffer = (unsigned short *)0xb8000;

