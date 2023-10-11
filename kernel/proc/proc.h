#pragma once
#include <stdint.h>
#include "../cpu/cpu.h"
#include "../cpu/idt.h"

typedef struct process_s{
    uint32_t memory_base;
    uint32_t memory_limit;
    uint32_t stack_base;
    uint32_t stack_limit;
    irq_registers_t regs;
    
    uint32_t allocated_pages;
    uint8_t lock;
    uint8_t priority;
    uint8_t abi;
    char *stdin;
    char *stdout;
    
}process32_t;