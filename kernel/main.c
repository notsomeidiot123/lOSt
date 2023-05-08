#include "graphics/vga.h"
#include "cpu/idt.h"
#include "cpu/cpu.h"
#include "drivers/timer.h"
#include "drivers/ps2.h"
#include "memory/mmanager.h"
#include "drivers/floppy.h"
#include "drivers/serial.h"
#include "memory/string.h"
#include "drivers/ata.h"


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
    // kprintf("Starting FDC:\n");
    // init_floppy();
    // kprintf("Floppy Disc Controller Initialization Finished\n");
    call_cpuid();
    char serial_res = serial_init();

    kprintf("Initializing Serial Ports\n");
    
    kprintf("Finished\n");
    // char *buf = kmalloc(1, 7);
    // padding = 2;
    // ata_identify();
    // ata_read28((unsigned short *)buf, 0, 0, 1);
    // for(int i = 0; i < 32; i++){
    //     for(int j = 0; j < 16; j++){
    //         kprintf("%x ", (unsigned char)buf[i + j]);
    //     }
    //     kprintf("\n");
    // }
    // short *test = (short *)0xb8000;
    // *test = 0x0f41;
};

//when we send the read command, we set a flag in the process struct, which prevents it from 
//being scheduled until cleared, and is cleared when the DMA process is finished reading. return from int and when finished, task switch
//back to the previous process