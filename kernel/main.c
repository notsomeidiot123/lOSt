#include "graphics/vga.h"
#include "cpu/idt.h"
#include "drivers/timer.h"
#include "drivers/ps2.h"
#include "memory/mmanager.h"

extern void kmain(void *mmap_ptr, short mmap_count, short mmap_type){
    set_color(0x7);
    clear_screen();
    kprintf("[      ] Loading IDT");
    init_idt();
    irq_install_handler(irq0_timer_handler, 0);
    master_pic_mask.data.pit = 0;
    // pic_remask();
    kputs("\r[ DONE ]\n[      ] Initializing 8042 Keyboard Controller");
    init_8042();
    master_pic_mask.data.ps2kb = 0;
    irq_install_handler(ps2_handler, 1);
    kprintf("\r[ DONE ]\n");
    pic_remask();
    kprintf("Registering MMAP:\n");
    init_memory(mmap_ptr, mmap_count); //make sure to adjust for other mmap types
    // kprintf("\ntest: count: %x, ptr: %x, type: %x", mmap_count, mmap_ptr, mmap_type);
};