#include "../graphics/vga.h"
#include "../cpu/io.h"
#include "../cpu/idt.h"

#define KB_DATA 0x60
#define KB_STATUS 0x64
#define KB_COMMAND 0x64

typedef union{
    unsigned char byte;
    struct{
        unsigned char int1:1;
        unsigned char int2:1;
        unsigned char sysflag:1;
        unsigned char zero:1;
        unsigned char portclk1:1;
        unsigned char portclk2:1;
        unsigned char translation:1;
        unsigned char zero0;
    }data;
}kb_status_u;

char *error_codes[] = {
    "Clock line stuck low",
    "Clock line stuck high",
    "Data line stuck low",
    "Data line stuck high",
};

int init_8042(){
    outb(KB_COMMAND, 0xAD);
    outb(KB_COMMAND, 0xA7);
    while (inb(KB_STATUS) & 1) {
        inb(KB_DATA);
    }
    outb(KB_COMMAND, 0x20);
    while(!(inb(KB_STATUS) & 1));
    kb_status_u stat = {0};
    stat.byte = inb(KB_DATA);
    stat.data.int1 = 0;
    stat.data.int2 = 0;
    stat.data.translation = 0;
    outb(KB_COMMAND, 0x60);
    outb(KB_COMMAND, stat.byte);
    outb(KB_COMMAND, 0xaa);
    while(!(inb(KB_STATUS)&1));
    if(inb(KB_DATA) != 0x55){
        kprintf("PS_2 Keyboard Initialization failed!\n");
        return 1;
    }
    outb(KB_COMMAND, 0xab);
    while(!(inb(KB_STATUS) & 1));
    unsigned char test_result = inb(KB_DATA);
    if(test_result != 0){
        kprintf("Error: PS_2 Keyboard Encountered an error: %s\n", error_codes[test_result - 1]);
    }
    if(!stat.data.portclk2){
        outb(KB_COMMAND, 0xa9);
        while(!(inb(KB_STATUS) & 1));
        test_result = inb(KB_DATA);
        if(test_result != 0){
            kprintf("Error: PS_2 Mouse Encountered an error: %s\n", error_codes[test_result - 1]);
        }
        outb(KB_COMMAND, 0xA8);
    }
    outb(KB_COMMAND, 0xAE);

    return 0;
}

char *translated_kb_codes = 
"\0\e1234567890-=\b\tqwertyuiop[]\n\x01\x61sdfghjkl;\'`\x02\\zxcvbnm,./\x03*\x04 \x05\x90\x91\x92\x93\x94\x95\x95\x96\x97\x98\x99\xa0\xa1\xc7\xc8\xc9\xbd\xc4\xc5\xc6\xbb\xc1\xc2\xc3\xc0\xbe\0\0\0\x9a\x9b\0\0\0";


void ps2_handler(irq_registers_t *regs){
    int scancode = inb(KB_DATA);
    if(scancode < 0x80){
        kprintf("%c", translated_kb_codes[scancode]);
    }
}