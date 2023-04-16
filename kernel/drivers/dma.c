#define BASE_0_3 0
#define BASE_4_7 0xc0
#define COUNT_OFFSET 2

#include "../cpu/io.h"

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
};

void mask_dma(char mask, char channel){
    outb(SINGLE_CHANNEL_MASK + (channel > 3 ) * BASE_4_7, mask);
}