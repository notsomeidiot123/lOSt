#pragma once
#include "../libs/types.h"
extern char serial_init();
extern void kputc(char c);
extern void kputs(char *s);
extern void kputd(int num);
extern void kputx(int num);
extern void kputb(int num);
extern void kprintf(char *format, ...);

void serial_write(int port, uint8_t c);
uint8_t serial_read(int port);
uint8_t register_serial_listener(void *port);
void deregister_serial_listener(void *port, uint8_t key);

extern void set_color(int color);
extern int get_color();
extern void clear_screen();

extern int padding;