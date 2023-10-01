#include "../memory/mmanager.h"
#include "../graphics/vga.h"
#include "../memory/string.h"
#include "proc.h"
#include "scheduler.h"

#define STK_SZ_PAGES 4096

uint16_t cpid = 0;//current scheduled pid
uint16_t lapid= 0;//last assigned pid



proc_list_t *proc_list[65536] = {0};

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
    uint32_t stack_size_cpy = proc->process->stack_limit - proc->process->regs.esp;
    nproc->stack_base = (uint32_t)(long)kumalloc(STK_SZ_PAGES, 6, ret);
    nproc->stack_limit = STK_SZ_PAGES * PG_SZ;
    nproc->regs.esp = (nproc->regs.esp - proc->process->stack_limit) + nproc->stack_limit;
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
    }
    return ret;
}
void exit_proc(uint32_t exit_code, uint32_t pid){
    kfree_pid(pid, get_proc(pid)->process->allocated_pages);
    kfree(get_proc(pid)->process);
    kfree(get_proc(pid));
    proc_list[pid] = 0;
}

//Replaces current process with code in the current buffer, while retaining PID data.
//Returns 0 on success, otherwise, returns -1
uint32_t exec(uint32_t *buffer, uint32_t buffer_size, uint16_t pid){
    //TODO: load new EFLAGS on start
    proc_list_t *proc_list_ent = get_proc(pid);
    uint32_t exists = (long)proc_list_ent;
    process32_t proc = *proc_list_ent->process;
    if(proc.memory_limit - proc.memory_base > buffer_size){
        kfree((void *)(long)proc.memory_base);
        proc.memory_base = (uint32_t)(long)kumalloc((long)buffer/4096, 7, pid);
        proc.memory_limit = proc.memory_base + buffer_size;
    }
    if(!proc.memory_base){
        return -1;
    }
    proc.regs.eip = proc.memory_base;
    proc.regs.esp = proc.stack_base;
    proc.regs.ebp = proc.stack_base;
    kmemcpy((void*)buffer, (void *)(long)proc.memory_base, buffer_size);
    return 0;
}