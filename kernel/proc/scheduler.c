#include "../memory/mmanager.h"
#include "../graphics/vga.h"
#include "../memory/string.h"
#include "proc.h"
#include "scheduler.h"
#include "../cpu/idt.h"
#include <stdint.h>
#define STK_SZ_PAGES 1



uint16_t cpid = 0;//current scheduled pid
uint16_t lapid= 0;//last assigned pid
uint32_t active_procs = 0;
proc_list_t **proc_list = 0;
uint8_t enable_scheduler = 0;


uint32_t (*load_exe)(FILE *file, uint32_t argc, char **argv);

uint32_t get_pid(){
    return cpid;
}

proc_list_t *get_proc(uint32_t pid){
    if( pid < 0 || pid > 65536){
        return 0;
    }
    return proc_list[pid];
}

uint32_t register_process(proc_list_t *parent, process32_t *process){
    proc_list_t *new_list_ent = kmalloc(1, 6);
    if(new_list_ent == 0){
        return -1;
    }
    new_list_ent->process = process;
    while(proc_list[lapid]){
        ++lapid;
    }
    new_list_ent->pid = lapid;
    new_list_ent->ppid = parent == 0 ? 0 : parent->pid;
    proc_list[lapid] = new_list_ent;
    return new_list_ent->pid;
}
//creates exact copy of process, returns 0 for child, returns PID for parent.
uint32_t fork(proc_list_t *proc){
    //TODO: create and load LDT and segments
    process32_t *nproc = kmalloc(1, 6);
    uint32_t ret = register_process(proc, nproc);
    if(!proc){
        kprintf("[Kernel Error]: Cannot Fork; Empty process\r\n");
        return 0;
    }
    uint32_t start = proc->process->memory_base;
    uint32_t size = proc->process->memory_limit - proc->process->memory_base;
    uint32_t *buffer = kumalloc(proc->process->memory_limit - proc->process->memory_base/4096, 7, ret);
    if(!buffer){
        kprintf("[Kernel Error]: Cannot Fork; Error Allocating Memory!\n\r");
        return 0;
    }
    //copy code
    kmemcpy((void*)(long)proc->process->memory_base, (void *)buffer, 1);
    //copy stack
    uint32_t stack_size_cpy = proc->process->stack_limit - proc->process->regs.useresp;
    nproc->stack_base = (uint32_t)(long)kumalloc(STK_SZ_PAGES, 6, ret);
    nproc->stack_limit = STK_SZ_PAGES * PG_SZ;
    nproc->regs.useresp = (nproc->regs.useresp - proc->process->stack_limit) + nproc->stack_limit;
    nproc->regs.ebp = (nproc->regs.ebp - proc->process->stack_limit) + nproc->stack_limit;
    kmemcpy((void*)(long)proc->process->stack_base, (void*)(long)nproc->stack_base, stack_size_cpy);
    
    nproc->priority = proc->process->priority;
    nproc->regs = proc->process->regs;
    nproc->regs.eip = nproc->memory_base + (proc->process->regs.eip - start);//find the offset into the code
    nproc->memory_base = (long long) buffer;
    nproc->memory_limit = (long long) buffer + size;
    nproc->regs = proc->process->regs;
    nproc->regs.eax = 0;
    nproc->stdin = kmalloc(1, 6);
    nproc->stdout = kmalloc(1, 6);
    nproc->priority = proc->process->priority;
    nproc->lock = 0;
    
    if(ret == -1){
        kfree(nproc);
        return 0;
    }
    active_procs++;
    return ret;
}
void exit_proc(uint32_t exit_code, uint32_t pid){
    kfree_pid(pid, get_proc(pid)->process->allocated_pages);
    kfree(get_proc(pid)->process);
    kfree(get_proc(pid));
    proc_list[pid] = 0;
    active_procs--;
}

//Replaces current process with code in the current buffer, while retaining PID data.
//Returns 0 on success, otherwise, returns -1

void exit_v(){
    uint32_t pid = get_pid();
    kfree_pid(pid, get_proc(pid)->process->allocated_pages);
    kfree(get_proc(pid)->process);
    kfree(get_proc(pid));
    proc_list[pid] = 0;
    active_procs--;
    for(;;);
}

uint32_t exec(uint32_t *buffer, uint32_t buffer_size, uint16_t pid, char *argv[]){
    //TODO: load new EFLAGS on start
    proc_list_t *proc_list_ent = get_proc(pid);
    uint32_t exists = (long)proc_list_ent;
    process32_t proc = *proc_list_ent->process;
    //called from kernel
    if(pid == 0){
        
    }
    if(proc.memory_limit - proc.memory_base > buffer_size){
        kfree((void *)(long)proc.memory_base);
        proc.memory_base = (uint32_t)(long)kumalloc((long)buffer/4096, 7, pid);
        proc.memory_limit = proc.memory_base + buffer_size;
    }
    if(!proc.memory_base){
        return -1;
    }
    proc.regs.eip = proc.memory_base;
    proc.regs.esp = proc.stack_limit;
    proc.regs.ebp = proc.stack_limit;
    proc.regs.useresp -= sizeof(int *) * 3 + sizeof(int);
    uint32_t *stack = (void *)(long)proc.stack_limit;
    stack[-2] = (uint32_t)(long)argv;
    uint32_t argc = 0;
    for(argc = 0; argv[argc] || argc < 65536; argc++);
    stack[-3] = argc;
    stack[-1] = (uint32_t)(long)exit_v;
    stack[0] = proc.regs.ebp;
    
    kmemcpy((void*)buffer, (void *)(long)proc.memory_base, buffer_size);
    return 0;
}

void init_scheduler(){
    //honestly, what else has to be done here?
    proc_list = kmalloc((sizeof(proc_list_t *) * 65536 )/ 4096, 6);
    if(proc_list == 0){
        kprintf("[lOSt]: Error initializing scheduler!\n");
        for(;;);
    }
    for(int i = 0; i < 65536; i++){
        proc_list[i] = 0;
    }
    proc_list[0] = (void*)-1;
    enable_scheduler = 1;
    return;
}

void p_push(uint32_t value, proc_list_t *proc){
    // proc->process->regs.esp -= 4;
    // *(uint32_t *)(long)proc->process->regs.esp = value;
    // kprintf("esp: %x\n", proc->process->regs.esp);
}

uint32_t kfork(void (*function)(), uint32_t args[], uint32_t count){
    // function();
    process32_t *proc = kmalloc(1, 6);
    uint16_t pid = register_process(0, proc);
    proc_list_t *proc_l = get_proc(pid);
    proc_l->process = proc;
    proc_l->flags.ring0 = 1;
    proc->abi = 0;
    proc->allocated_pages = -1;
    proc->lock = 0;
    proc->priority = 4;
    proc->stack_base = (uint32_t)(long)kumalloc(1, 6, pid);
    proc->regs.eip = (uint32_t)(long)function;
    proc->stack_limit = proc->stack_base + 4096;
    proc->regs.esp = (uint32_t)proc->stack_limit;
    proc->regs.ebp = (uint32_t)proc->stack_limit;
    proc->regs.cs = 0x8;
    proc->regs.ds = 0x10;
    proc->regs.ss = 0x10;
    proc->regs.es = 0x10;
    proc->regs.eflags = get_eflags();
    kprintf("ESP: %x, ARGV: %x, ARGC: %x\n", proc->regs.esp, args, count);
    // for(int i = 0; i < count; i++){
    //     // *((uint32_t *)(long)(proc->regs.esp + i * sizeof(uint32_t))) = args[i];
    // }
    proc->regs.esp = push_args(args, count, proc->regs.esp) - 4;
    //fix... whatever this is
    //note to self: arguments are a mess...
    kprintf("Count: %x", count * 4);
    
    // proc->regs.ebp = proc->regs.esp;
    active_procs++;
    return pid;
}
char first_run = 0;
//TODO: Upgrade to support SMP
//DON'T TOUCH IT, IT FINALLY WORKS
irq_registers_t *schedule(irq_registers_t *oldregs){
    if(active_procs == 0 || enable_scheduler == 0){
        return oldregs;
    }
    proc_list[cpid]->process->regs = *oldregs;
    cpid++;
    while(!proc_list[cpid] || (!proc_list[cpid] && !proc_list[cpid]->process->lock)){
        cpid++;
    }
    if(proc_list[cpid]->flags.ring0){
        oldregs = (void *)(proc_list[cpid]->process->regs.esp - sizeof(irq_registers_t));
        *oldregs = proc_list[cpid]->process->regs;
        return oldregs;
    }
    *oldregs = proc_list[cpid]->process->regs;
    return oldregs;
    // kprintf("ebp:%x\n", oldregs->esp);
}