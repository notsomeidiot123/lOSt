#include "../cpu/io.h"
#include "../cpu/idt.h"


unsigned int seconds;
unsigned int ticks;

void irq0_timer_handler(){
    ticks++;
    if(ticks % 18 == 0){
        seconds++;
    }
}

int wait_secs(int count){
    static int start;
    if(count != 0){
        start = seconds + count - 1;
        return seconds > start;
    }
    else if(seconds > start){
        return 1;
    }
    return 0;
}