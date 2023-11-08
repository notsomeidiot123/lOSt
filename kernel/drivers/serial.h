#pragma once
#include "../libs/types.h"
extern char serial_init();
extern void kputc(char c);
extern void kputs(char *s);
extern void kputd(int num);
extern void kputx(int num);
extern void kputb(int num);
extern void kprintf(char *format, ...);

extern void serial_write(int port, uint8_t c);
extern uint8_t serial_read(int port);
extern uint32_t register_serial_listener(void *listener, int port, int pid);
extern void deregister_serial_listener(int port, uint32_t key);


extern void clear_screen();
extern void toggle_auto_return();

extern int padding;