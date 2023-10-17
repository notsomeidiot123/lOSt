#include "../cpu/io.h"
#include "../cpu/idt.h"
#include "../proc/scheduler.h"

unsigned int seconds;
unsigned int ticks;

void irq0_timer_handler(irq_registers_t *registers){
    ticks++;
    if(ticks % 18 == 0){
        seconds++;
        // schedule(registers);   
        schedule(registers);
    }
    // schedule(registers);
    
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
int wait_ticks(int count){
    static int start;
    if(count != 0){
        start = ticks + count - 1;
        return ticks > start;
    }
    else if(ticks > start){
        return 1;
    }
    return 0;
}