#include "timer.h"
#include "../graphics/vga.h"
#include "../memory/mmanager.h"
#include "storage.h"
#include "../cpu/io.h"

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

int init_floppy(){
    outb(DIGITAL_OUTPUT, 0x34);//turn on motors A and B and enter normal operation mode
    return 0;
}