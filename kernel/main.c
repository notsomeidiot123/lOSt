#include "graphics/vga.h"
#include "cpu/idt.h"
#include "drivers/timer.h"
#include "drivers/ps2.h"

extern void kmain(void *mmap_ptr, short mmap_count, char mmap_type){
    set_color(0x80);
    clear_screen();
    kputs("Hello, World!\n");
    init_idt();
    irq_install_handler(irq0_timer_handler, 0);
    init_8042();
    master_pic_mask.data.pit = 0;
    master_pic_mask.data.ps2kb = 0;
    pic_remask();
    
}
