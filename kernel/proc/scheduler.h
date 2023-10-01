#pragma once
#include "proc.h"
#include <stdint.h>

typedef struct proc_list_s{
    process32_t *process;
    uint16_t pid;
    uint16_t ppid;
    uint16_t uid;
}proc_list_t;
proc_list_t *get_proc(uint32_t pid);
uint32_t fork(proc_list_t *proc);
void exit_proc(uint32_t exit_code, uint32_t pid);