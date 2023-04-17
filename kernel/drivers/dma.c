#define BASE_0_3 0
#define BASE_4_7 0xc0
#define CHANNEL(a) a * 2
#define COUNT + 1




#include "../cpu/io.h"
#include "../memory/mmanager.h"

enum DMARegs{
    STATUS = 8,
    COMMAND = 8,
    REQUEST,
    SINGLE_CHANNEL_MASK,
    MODE,
    FLIP_FLOP_RESET,
    INTERMED,
    MASTER_RESET = 0xD,
    MASK_RESET,
    MULTI_MASK,
    CHANNEL_2_PAGE=0x81,
    CHANNEL_3_PAGE,
    CHANNEL_1_PAGE,
    CHANNEL_0_PAGE = 0x87,
    CHANNEL_6_PAGE = 0x89,
    CHANNEL_7_PAGE,
    CHANNEL_5_PAGE,
    CHANNEL_4_PAGE = 0x8f
};

inline void mask_dma(char mask, char channel){
    outb(SINGLE_CHANNEL_MASK + (channel > 3 ) * BASE_4_7, mask);
}

void *init_floppy_dma(){
    void *ptr = kmalloc(4, MEMORY_PERM_READ | MEMORY_PERM_WRTE);
    outb(SINGLE_CHANNEL_MASK, 0x6); //mask port 2 and port 0
    outb(FLIP_FLOP_RESET, 0xff); //reset
    outb(BASE_0_3 + CHANNEL(2), (long)ptr & 0xff);  //base of the pointer
    outb(BASE_0_3 + CHANNEL(2), ((long)ptr & 0xff00) >> 8);//base of the pointer
    outb(FLIP_FLOP_RESET, 0xff);
    outb(BASE_0_3 + CHANNEL(2) COUNT, 0xff);//allow for one track of data
    outb(BASE_0_3 + CHANNEL(2) COUNT, 0x23);//allow for one track of data
    outb(CHANNEL_2_PAGE, ((long)ptr & 0xff0000) >> 16);
    outb(SINGLE_CHANNEL_MASK, 0x2);
    return ptr;
}