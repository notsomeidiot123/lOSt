#include "drivers/storage.h"
#include "graphics/vga.h"
#include "cpu/idt.h"
#include "cpu/cpu.h"
#include "drivers/timer.h"
#include "drivers/ps2.h"
#include "memory/mmanager.h"
#include "cpu/ecodes.h"
#include "drivers/floppy.h"
#include "drivers/serial.h"
#include "memory/string.h"
#include "drivers/ata.h"
#include "cpu/ksec.h"

char catstr[60];

extern void kmain(void *mmap_ptr, short mmap_count, short mmap_type){
    textmode_print_load();
    set_color(0x7);
    char serial_res = serial_init();
    kprintf("Finished\n");
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
    kstrcat(catstr, "RAM: ", ltostr(get_ram_size()/1024/1024, 10, 0));
    kstrcat(catstr, catstr, "Mb");
    disp_str(40, 12, catstr);
    // kprintf("Starting FDC:\n");
    // init_floppy();
    // kprintf("Floppy Disc Controller Initialization Finished\n");
    call_cpuid();
    // kprintf("[      ] Identifying PATA Drives");
    ata_identify_all();
    // kprintf("\r[%s]\n", ata_res ? "ERROR!" : " DONE ");
    
    disp_str(40, 13, "Finished in Time:");
    disp_str(40 + 17/2 + 4, 13, ltostr(seconds, 10, 0));
};

//when we send the read command, we set a flag in the process struct, which prevents it from 
//being scheduled until cleared, and is cleared when the DMA process is finished reading. return from int and when finished, task switch
//back to the previous process