#pragma once
#include "proc.h"
#include <stdint.h>

//TODO: modularize all of this

typedef struct proc_list_s{
    process32_t *process;
    uint16_t pid;
    uint16_t ppid;
    uint16_t uid;
}proc_list_t;



proc_list_t *get_proc(uint32_t pid);
uint32_t fork(proc_list_t *proc);
uint32_t get_pid();
uint32_t exec(uint32_t *buffer, uint32_t buffer_size, uint16_t pid, char *argv[]);
void schedule(irq_registers_t *oldregs);
void exit_proc(uint32_t exit_code, uint32_t pid);
void kfork(void (*function)(), uint32_t args[], uint32_t count);