#include "drivers/storage.h"
#include "filesystems/fat.h"
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
#include "eshell/eshell.h"
#include "proc/scheduler.h"

char catstr[60];

#define MAX_KERNEL_SIZE 0x400 * 512

extern void start;
extern void end;

//IDLE process must exist for scheduling to work properly?
//idk it's weird

void idle(){
    for(;;);
}
void argtest(uint32_t e, uint32_t test, uint32_t test2){
    int testv = 0;
    kprintf("ARG1: %x, ARG2: %x, DIFF: %x\nVAR: %x, EBP: %x, ESP: %x", &test2, &test, &test - &test2, &testv, get_proc(get_pid())->process->regs.ebp, get_proc(get_pid())->process->regs.ebp);
    kprintf("\nVARG1: %d, VARG2: %d, E: %d\n", test2, test, e);
    for(;;);
    exit_v();
}
extern void kmain(void *mmap_ptr, short mmap_count, short mmap_type){
    init_memory(mmap_ptr, mmap_count); //make sure to adjust for other mmap types
    kprintf("Registering MMAP:\n");
    kstrcat(catstr, "RAM: ", ltostr(get_ram_size()/1024/1024, 10, 0));
    kstrcat(catstr, catstr, "Mb");
    disp_str(40, 12, catstr);
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
    // kprintf("Starting FDC:\n");
    // init_floppy();
    // kprintf("Floppy Disc Controller Initialization Finished\n");
    call_cpuid();
    // kprintf("[      ] Identifying PATA Drives");
    ata_identify_all();
    // kprintf("\r[%s]\n", ata_res ? "ERROR!" : " DONE ");
    // fopen("A:/test.txt", MODE_WRITE);
    FILE *lostrc = 0;
    kprintf("Size of Kernel: %d\n", &end - &start);
    if(&end - &start > MAX_KERNEL_SIZE){
        kprintf("lOSt Developer Warning: HEY!!! DUMMY! THE KERNEL'S NOT BEING FULLY LOADED, GO UPDATE THE BOOTLOADER >:(\n");
        disp_str(40, 0, "lOSt Developer Warning: HEY! THE KERNEL'S TOO LARGE, LOOK AT THE MONITOR!");
        while(1);
    }
    kprintf("[      ] Initializing Scheduler");
    init_scheduler();
    kprintf("\r[ DONE ]\n");
    kprintf("finished\n");
    disp_str(40, 13, "Finished in Time:");
    disp_str(40 + 17/2 + 4, 13, ltostr(seconds, 10, 0));
    if(lostrc == 0){
        disp_str(40, 14, "Error: Cannot find lOSt.rc on any mounted filesystem");
        disp_str(40, 15, "Booting into emergency shell!");
        // eshell();
        kprintf("PTR: %x", eshell);
        uint32_t shell_pid = kfork(eshell, 0, 0);
        kfork(idle, 0, 0);
        kfork(argtest, (uint32_t []){0,10}, 2);
        kprintf("forked\n Active Procs: %d, PID: %d\n", active_procs, shell_pid);
    }
    // kfork(idle, 0, 0);
};

//when we send the read command, we set a flag in the process struct, which prevents it from 
//being scheduled until cleared, and is cleared when the DMA process is finished reading. return from int and when finished, task switch
//back to the previous process