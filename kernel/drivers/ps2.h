#pragma once
#include "../cpu/idt.h"

extern char last_char;
extern char ps_2_interrupt_fired;

extern int init_8042();
void ps2_handler(irq_registers_t *regs);
char getchar();