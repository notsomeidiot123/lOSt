#include "timer.h"
#include "../graphics/vga.h"
#include "../memory/mmanager.h"
#include "storage.h"
#include "../cpu/io.h"
#include "../cpu/idt.h"
#include "dma.h"

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
    SCAN_HEQ = 29,
};

#define LOCK_BIT 0x80

/*

REMINDER ABOUT THE FDC SUBSYSTEM: ALL COMMANDS AND DATA GOES THROUGH THE DATA_FIFO REGISTER!!!
Issuing a command goes outb(DATA_FIFO, {command});
Reading result/data goes inb(DATA_FIFO);

OR the command with 0x80 for multitrack mode, useful for large rw commands
ALWAYS set 0x40 when reading/writing/formatting/verifying

*/
void lba_to_chs(int lba, int *cyl, int *head, int *sector){
    *cyl = lba/ (2 * 18);
    *head = ((lba % ( 2 * 18)) / 18);
    *sector = ((lba % (2 * 18)) % 18 + 1);
}

char irq_hit = 0;
char locked_cal = 0;

void floppy_irq_handler(irq_registers_t *regs){
    // kprintf("Floppy IRQ Hit!\n");
    irq_hit = 1;
}

int reset_controller();

void sense_interupt(){
    outb(DATA_FIFO, SENSE_INTERRUPT);
    inb(DATA_FIFO);
    inb(DATA_FIFO);
}

void recal_fdc(){
    for(int i = 0; i < 2; i++){
        outb(DIGITAL_OUTPUT, (inb(DIGITAL_OUTPUT) & 0xfc) + i);
        while( (inb(MSR) & 0xc0) != 0x80 ){
            reset_controller();
        }
        outb(DATA_FIFO, RECALIBRATE);
        while( (inb(MSR) & 0xc0) != 0x80 ){
            reset_controller();
        }
        outb(DATA_FIFO, RECALIBRATE);
    }
    // sense_interupt();
}


int reset_controller(){
    irq_hit = 0;
    unsigned char val = inb(DIGITAL_OUTPUT);
    outb(DIGITAL_OUTPUT, 0x00);
    outb(DIGITAL_OUTPUT, 0x0C);
    while(!irq_hit) /*kprintf("Waiting\n")*/;
    if(!locked_cal){
        recal_fdc();
    }
    // for(int i = 0; i < 4096; i++);
    // kprintf("end");
    return 0;
}

char *dma_data_ptr;
int init_floppy(){
    irq_install_handler(floppy_irq_handler, 6);
    master_pic_mask.data.floppy = 0;
    pic_remask();
    while( (inb(MSR) & 0xc0) != 0x80 ){
        reset_controller();
    }
    outb(CONFIG_CONTROL, 0); //set datarate to match 1.44/1.2MB drives
    outb(DIGITAL_OUTPUT, 0x3c); //turn on drive motors, enter normal 
    while(!(inb(MSR) & 0x80));
    outb(DATA_FIFO, VERSION);
    while(!(inb(MSR) & 0x80));
    unsigned char version = inb(DATA_FIFO);
    // kprintf("FDC Controller Version: 0x%x", version);
    if(version != 0x90){
        kprintf("\tAborting!\n");
        return 0;
    }

    
    while( (inb(MSR) & 0xc0) != 0x80 ){
        reset_controller();
    }
    outb(DATA_FIFO, CONFIG);
    while(!(inb(MSR) & 0x80));
    outb(DATA_FIFO, 0);
    while(!(inb(MSR) & 0x80));
    outb(DATA_FIFO, 1 << 6 | 0 << 5 | 0 << 4 | 8);
    while(!(inb(MSR) & 0x80));
    outb(DATA_FIFO, 0);
    
    // kprintf("pre spec");
    while( (inb(MSR) & 0xc0) != 0x80 ){
        reset_controller();
        outb(DATA_FIFO, SPECIFY);
    }
    outb(DATA_FIFO, 8 << 4 | 0);
    outb(DATA_FIFO, 5 << 1 | 0);
        
    while( (inb(MSR) & 0xc0) != 0x80 ){
        // kprintf("hang");
        reset_controller();
    }
    // kprintf("pre recal");
    recal_fdc();
    outb(DATA_FIFO, LOCK | LOCK_BIT);
    locked_cal = 1;
    reset_controller();
    // kprintf("pre dma init");
    dma_data_ptr = init_floppy_dma();
    
    outb(DIGITAL_OUTPUT, inb(DIGITAL_OUTPUT) & 0xfc); //go back to drive select one
    kprintf("DMA PTR: 0x%x!\n", dma_data_ptr);

    return 0;
}

void floppy_mot_on(char drive){
    int dor = inb(DIGITAL_OUTPUT);
    dor &= 0x0f;
    dor |= 1 << (4 + (drive % 4));
    outb(DIGITAL_OUTPUT, dor);
}

char *floppy_read(int LBA, char drive){
    irq_hit = 0;
    int cyl = 0;
    int head = 0;
    int sector = 0;

    lba_to_chs(LBA, &cyl, &head, &sector);

    char dor = inb(DIGITAL_OUTPUT);

    dor &= 0x0c;
    outb(DIGITAL_OUTPUT, dor | (drive % 4));
    outb(CONFIG_CONTROL, 0);
    floppy_mot_on(drive);
    char command_seq[] = {
        head << 2 | drive,
        cyl,
        head << 2 | drive,
        sector,
        2,
        18,
        0x1b,
        0xff,
    };
    while( (inb(MSR) & 0xc0) != 0x80 ){
        reset_controller();
    }
    
    outb(DATA_FIFO, READ_DATA | 0x80 | 0x40);
    for(int i = 0; i < 8; i++){
        wait_secs(3);
        while( (inb(MSR) & 0xc0) != 0x80 ){
            if(wait_secs(0)){
                reset_controller();
                return 0;
            }
        }
        outb(DATA_FIFO, command_seq[i]);
    }
    
    
    // while(!irq_hit) /*kprintf("waiting for end! %x\n", inb(MSR))*/;
    
    
    while((inb(MSR) & 0x90)){
        // while((inb(MSR) & 0xc0) != 0xc0);
        kprintf( "%x ", inb(DATA_FIFO));
    }
    return dma_data_ptr;
}