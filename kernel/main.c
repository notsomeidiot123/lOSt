#include "graphics/vga.h"
#include "cpu/idt.h"

extern void kmain(void *mmap_ptr, short mmap_count, char mmap_type){
    set_color(0x80);
    clear_screen();
    kputs("Hello, World!\n");
    init_idt();
}
