#pragma once
#include "../cpu/idt.h"

extern char last_char;
extern char ps_2_interrupt_fired;

extern int init_8042();
extern void ps2_handler(irq_registers_t *regs);
extern char getchar();

extern unsigned char get_lastkey();