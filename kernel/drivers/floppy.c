#include "timer.h"
#include "../graphics/vga.h"
#include "../memory/mmanager.h"
#include "storage.h"
#include "../cpu/io.h"
#include "../cpu/idt.h"

#define CYL 80
#define HPD 2
#define SPT 18

enum FLOPPY_REGS{
    STAT_A = 0x3f0,
    STAT_B,
    DIGITAL_OUTPUT,
    TAPE_DRIVE,
    MSR,
    DATARATE_SELECT = 0x3F4,
    DATA_FIFO,
    DIGITAL_INPUT = 0x3f7,
    CONFIG_CONTROL = 0x3f7
};

enum FLOPPY_COMMANDS{
    READ_TRACK = 2,
    SPECIFY, 
    SENSE_DRIVE_STAT,
    WRITE_DATA,
    READ_DATA,
    RECALIBRATE,
    SENSE_INTERRUPT,
    WRITE_DELETED_DATA,
    READ_ID,
    READ_DELETED_DATA=12,
    FORMAT_TRACK,
    DUMPREG,
    SEEK,
    VERSION,
    SCAN_EQ,
    PERP_MODE,
    CONFIG,
    LOCK,
    VERIFY=22,
    SCAN_LEQ = 25,
    SCAN_HEQ = 29
};

/*

REMINDER ABOUT THE FDC SUBSYSTEM: ALL COMMANDS AND DATA GOES THROUGH THE DATA_FIFO REGISTER!!!
Issuing a command goes outb(DATA_FIFO, {command});
Reading result/data goes inb(DATA_FIFO);

OR the command with 0x80 for multitrack mode, useful for large rw commands
ALWAYS set 0x40 when reading/writing/formatting/verifying

*/

char irq_hit = 0;

void floppy_irq_handler(irq_registers_t *regs){
    irq_hit = 0;
}

int reset_controller(){
    return 0;
}

int init_floppy(){
    while( (inb(MSR) & 0xc) != 0x80 ){
        reset_controller();
    }
    outb(CONFIG_CONTROL, 0); //set datarate to match 1.44/1.2MB drives
    outb(DIGITAL_OUTPUT, 0x3c); //turn on drive motors, enter normal 
    outb(DATA_FIFO, VERSION);
    unsigned char version = inb(DATA_FIFO);
    kprintf("FDC Controller Version: 0x%x", version);
    if(version != 0x90){
        kprintf("\tAborting!\n");
        return 0;
    }

    irq_install_handler(floppy_irq_handler, 6);
    master_pic_mask.data.floppy = 0;
    pic_remask();
    return 0;
}