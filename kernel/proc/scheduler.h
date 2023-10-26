#pragma once
#include "proc.h"
#include "../drivers/storage.h"
#include <stdint.h>

//TODO: modularize all of this

typedef struct proc_list_s{
    process32_t *process;
    uint16_t pid;
    uint16_t ppid;
    uint16_t uid;
    struct proc_flags_s{
        uint8_t ring0:1;
    }flags;
    char *path_to_exec;
}proc_list_t;

extern uint32_t active_procs;
// extern uint32_t (*load_exe)(FILE *file, uint32_t argc, char **argv);
proc_list_t *get_proc(uint32_t pid);
uint32_t fork(proc_list_t *proc);
uint32_t get_pid();
uint32_t exec(uint32_t *buffer, uint32_t buffer_size, uint16_t pid, char *argv[]);
void init_scheduler();
irq_registers_t *schedule(irq_registers_t *oldregs);
void exit_proc(uint32_t exit_code, uint32_t pid);
uint32_t kfork(void (*function)(), uint32_t args[], uint32_t count);
extern int push_args(uint32_t *argv, uint32_t argc, uint32_t esp);
//WARNING: ONLY WORKS FOR KERNEL PROCESSES, DO NOT USE OR EXPOSE TO USER PROCESSES
void exit_v();