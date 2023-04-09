#include "../memory/string.h"
#include "../libs/math.h"
#include <stdarg.h>

typedef struct {
    short mode;
    char text: 1;
    char colors: 4;
    int xres;
    int yres;
    //0=1, 1=16, 2=256, and so on
}vga_descriptor_t;

unsigned short *vga_buffer = (unsigned short *)0xb8000;
int base_color;
int buffer_index = 0;
int xpos = 0;
int ypos = 0;
int padding = 1;

vga_descriptor_t video_mode = {0, 1, 0x1, 80, 25};

void clear_screen(){
    kmemset((short* )vga_buffer, video_mode.xres * video_mode.yres*2, base_color << 8);
    buffer_index = 0;
}

void kputc(char c){
    if(video_mode.colors == 1){
        char col = (char)base_color;
        switch (c) {
            case '\b':
                if(buffer_index <= 0){
                    buffer_index = 1;
                }
                vga_buffer[--buffer_index] = base_color << 8 | 0;
                break;
            case '\t':
                for(int i = 0; i < 4; i++){
                    kputc(' ');
                }
                break;
            case '\n':
                buffer_index += 80 - (buffer_index % 80);
                break;
            case '\r':
                buffer_index -= buffer_index % 80;
                break;
            case '\e':
                clear_screen();
                buffer_index = 0;
                break;
            case '\a':
                //bell
                break;
            default:
                vga_buffer[buffer_index++] = base_color << 8 | c;
                break;
        }

    }
    if(buffer_index >= 80*25){
        kmemcpy((short *)vga_buffer+80, (short *)vga_buffer, 80*25*2 - 160);
        kmemset((short *)vga_buffer+(80*24), 160, base_color);
        buffer_index = 80*24;
    }
    //implement scrolling with memcpy :) :finsihed:
}

void kputs(char *s){
    while(*s){
        kputc(*s);
        s++;
    }
}

void kputd(int num){
    int i = num < 0 ? -num : num;
    kputs(ltostr(i, 10, padding));
}

void kputx(unsigned int num){
    kputs(ltostr(num, 16, padding));
}

void kputb(int num){
    kputs(ltostr(num, 2, padding));
}

void kprintf(char *format, ...){
    char *str;
    int i;
    va_list arg;
    va_start(arg, format);
    for(char *fmt = format; *fmt != 0; fmt++){
        while(*fmt != '%'){
            if(*fmt == 0){
                return;
            }
            kputc(*fmt);
            fmt++;
        }
        fmt++;
        switch(*fmt){
            case 'c' : 
                i = va_arg(arg, int);
                kputc(i);
                break;
            case 'd':
                i = va_arg(arg, int);
                if(i < 0){
                    i = -i;
                    kputc('-');
                }
                kputd(i);
                break;
            case 'x':
                i = va_arg(arg, int);
                kputx(i);
                break;
            case 'o':
                i = va_arg(arg, int);
                kputs(ltostr(i, 8, 0));
                break;
            case 's':
                str = va_arg(arg, char *);
                kputs(str);
                break;

        }
    }
}


void set_color(int color){
    base_color = color;
}
int get_color(){
    return base_color;
}
