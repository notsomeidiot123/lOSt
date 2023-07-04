#include "../libs/math.h"
#include "../memory/string.h"

typedef struct {
    short mode;
    char text: 1;
    char colors: 4;
    int xres;
    int yres;
    //0=1, 1=16, 2=256, and so on
}vga_descriptor_t;

unsigned short *vga_buffer = (unsigned short *)0xb8000;

const char loadstr[] = "Loading lOSt";

void textmode_print_load(){
    int i = 0;
    int start = (80 * 11) + (80/2) - (13/2)-1;
    while(loadstr[i]){
        vga_buffer[start++] = loadstr[i++] | 0x02 << 8;
    }
}

extern void disp_str(int x, int y, char *str){
    int i = 0;
    int start = x + (y*80) - kstrlen(str)/2 - 1;
    while(str[i]){
        vga_buffer[start++] = str[i++] | 0x2 << 8;
    }
}