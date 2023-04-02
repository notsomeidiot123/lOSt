#pragma once
#include "../cpu/idt.h"
extern int init_8042();
void ps2_handler(irq_registers_t *regs);