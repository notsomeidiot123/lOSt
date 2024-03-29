#include "../graphics/vga.h"
#include "../cpu/io.h"
#include "../cpu/idt.h"
#include "kb.h"
#include "ps2.h"
#include <stdint.h>

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
    request_register_driver(register_handlers);
    return 0;
}

//special chars: 
/*
lctrl: 1
lshft: 2
rshft: 3
lalt : 4
caps : 5
f1-f10: 0x9x
numlk: 0xa0
scrlk: 0xa1
numpd: asciicode | 0x80
f11  : 0x9a
f12  : 0x9b
*/




uint8_t *translated_kb_codes = 
(uint8_t *)"\0\e1234567890-=\b\tqwertyuiop[]\n\x81\x61sdfghjkl;\'`\x82\\zxcvbnm,./\x82*\x83 \x84\x90\x91\x92\x93\x94\x95\x95\x96\x97\x98\x99\xa0\xa1\xb7\xb8\xb9\xbd\xb4\xb5\xb6\xbb\xb1\xb2\xb3\xb0\xbe\0\0\0\x9a\x9b\0\0\0";

char *shifted_nums = ")!@#$%^&*(";

char last_char = 0;
char ps_2_interrupt_fired = 0;
unsigned char last_key = 0;
unsigned char get_lastkey(){
    return last_key;
}
char getchar(){
    ps_2_interrupt_fired = 0;
    return last_char;
}

char get_ascii(char scanned, char shift, char numlock){
    if(shift){
        if(scanned >= 'a' && scanned <= 'z'){
            return scanned - 0x20;
        }
        else if(scanned >= '0' && scanned <= '9'){
            return shifted_nums[scanned - '0'];
        }
        else switch (scanned) {
            case '`':
                return '~';
            case '-':
                return '_';
            case '=':
                return '+';
            case ';':
                return ':';
            case '\'':
                return '"';
            case ',':
                return '<';
            case '.':
                return '>';
            case '/':
                return '?';
            case '[':
                return '{';
            case ']':
                return  '}';
            case '\\':
                return '|';
            default:
                return ' ';
        }
    }
    if(numlock && ((unsigned) scanned >= 0xb0) && ((unsigned) scanned <= 0xb9)){
        return scanned ^ 0x80;
    }
    return scanned;
}

char shift_down = 0;
char caps_lock = 0;
void (*ps2_listener)(uint8_t c) = 0;

void register_handlers(kb_handler translated_handler){
    ps2_listener = translated_handler;
    return;
}

irq_user_registers_t *ps2_handler(irq_user_registers_t *regs){
    int scancode = inb(KB_DATA);
    if(scancode < 0x80){
        switch(translated_kb_codes[scancode]){
            case LSHFT:
            case RSHFT:
                shift_down = 1;
                return regs;
                break;
            case CAPS:
                caps_lock = !caps_lock;
                return regs;
                break;
            case LALT:
                return regs;
                break;
            default:
                break;
        }
        ps_2_interrupt_fired = 1;
        last_key = scancode;
        last_char = get_ascii(translated_kb_codes[scancode], shift_down | caps_lock, 0);
        if(ps2_listener){
            kb_listener(last_char);
        }
    }
    else{
        switch(translated_kb_codes[scancode ^ 0x80]){
            case LSHFT:
            case RSHFT:
                shift_down = 0;
                return regs;
                break;
            case CAPS:
            case LALT:
                return regs;
                break;
            default:
                break;
        }
    }
    return regs;
}