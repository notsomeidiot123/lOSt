#include "idt.h"
#include "io.h"
#include "../drivers/ps2.h"
#include "../graphics/vga.h"
#include "../drivers/serial.h"
#include "../drivers/ps2.h"
#include "../memory/mmanager.h"
#include "../memory/string.h"
// #include "../filesystems/fat.h"
#include "../drivers/storage.h"
#include "../proc/scheduler.h"
#include "../drivers/timer.h"

uint32_t get_eflags(){
    uint32_t ret = 0;
    asm("pushfl;"\
    "popl %%eax;"\
    "movl %%eax, %0"\
    : "=m" (ret)
    );
    return ret;
}

uint8_t kernel_mode = 1;


extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
extern void softint();

extern void isr_stub();
extern void _isr0();
extern void _isr1();
extern void _isr2();
extern void _isr3();
extern void _isr4();
extern void _isr5();
extern void _isr6();
extern void _isr7();
extern void _isr8();
extern void _isr9();
extern void _isr10();
extern void _isr11();
extern void _isr12();
extern void _isr13();
extern void _isr14();
extern void _isr15();
extern void _isr16();
extern void _isr17();
extern void _isr18();
extern void _isr19();
extern void _isr20();
extern void _isr21();
extern void _isr22();
extern void _isr23();
extern void _isr24();
extern void _isr25();
extern void _isr26();
extern void _isr27();
extern void _isr28();
extern void _isr29();
extern void _isr30();
extern void _isr31();

typedef enum {
    IDT_FLAG_GATE_TASK          = 0x5,
    IDT_FLAG_GATE_16BIT_INT     = 0x6,
    IDT_FLAG_GATE_16BIT_TRAP    = 0x7,
    IDT_FLAG_GATE_32BIT_INT     = 0xE,
    IDT_FLAG_GATE_32BIT_TRAP    = 0xF,

    IDT_FLAG_RING0              = (0 << 5),
    IDT_FLAG_RING1              = (1 << 5),
    IDT_FLAG_RING2              = (2 << 5),
    IDT_FLAG_RING3              = (3 << 5),

    IDT_FLAG_PRESENT            = 0x80,
} IDT_FLAGS;


struct idt_entry_s{
    unsigned short offset_low;
    unsigned short segment_selector;
    char reserved;
    struct idt_flags_s{
        unsigned char gate_type: 4;
        unsigned char zero:1;
        unsigned char dpl:2;
        unsigned char present:1;
    }idt_flags;
    unsigned short offset_high;
}idt_entries[256];

void idt_set(unsigned char num, unsigned int address, struct idt_flags_s flags){
    idt_entries[num].offset_low = address & 0xffff;
    idt_entries[num].offset_high = (address >> 16) & 0xffff;
    idt_entries[num].reserved = 0;
    idt_entries[num].idt_flags = flags;
    idt_entries[num].segment_selector = 0x8;//kernel code segment
}

extern void load_cs();

short loaded_code_segment = 0x8;
short loaded_data_segment = 0x10;

void load_code_segment(short segment){
    loaded_code_segment = segment;
    load_cs();
}


extern void panic(irq_registers_t *regs, ...){
    char *exceptions[] = {
        "Divide by 0",
        "Reserved",
        "NMI Interrupt",
        "Breakpoint",
        "Overflow",
        "Bounds range exceeded",
        "Invalid Opcode",
        "Device not Available",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Invalid TSS",
        "Segment not Present",
        "Stack-segment",
        "General Protection",
        "Page Fault",
        "Reserved",
        "x87 FPU error",
        "Alignment Check",
        "Machine Check",
        "SIMD FP",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Software Panic!",
    };
    char *opcodes[] = {
        "ADD", "ADD", "ADD", "ADD", "ADD AL, ", "ADD EAX, ", "PUSH ES", "POP ES", "OR", "OR", "OR", "OR", "OR AL, ",
        "OR EAX, ", "PUSH CS", "0x0F", "ADC", "ADC", "ADC", "ADC", "ADC AL,", "ADC EAX, ", "PUSH SS", "POP SS",
        "SBB", "SBB", "SBB", "SBB", "SBB AL,", "SBB EAX,", "PUSH DS", "POP DS",
        "AND", "AND", "AND", "AND", "AND AL,", "AND EAX,", "ES:", "DAA", "SUB", "SUB", "SUB", "SUB", "SUB AL,", "SUB EAX,", "CS:", "DAS",
        "XOR", "XOR", "XOR", "XOR", "XOR AL,", "XOR EAX,", "SS:", "AAA", "CMP", "CMP", "CMP", "CMP", "CMP AL, ", "CMP EAX,", "DS:", "AAS",
        "INC EAX", "INC ECX", "INC EDX", "INC EBX", "INC ESP", "INC EBP", "INC ESI", "INC EDI", "DEC EAX", "DEC ECX", "DEC EDX", "DEC EBX", "DEC ESP", "DEC EBP", "DEC ESP", "DEC EDI",
        "PUSH EAX", "PUSH ECX", "PUSH EDX", "PUSH EBX", "PUSH ESP", "PUSH EBP", "PUSH ESI", "PUSH EDI", "POP EAX", "POP ECX", "POP EDX", "POP EBX", "POP ESP", "POP EBP", "POP ESI", "POP EDI",
        "PUSHA", "POPA", "BOUND", "ARPL", "FS:", "GS:", "OPSIZE:", "ADSIZE", "PUSH", "IMUL", "PUSH", "IMUL", "INSB", "INSW", "OUTSB",  "OUTSW",
        "JO", "JNO", "JB", "JNB", "JZ", "JNZ", "JBE", "JA", "JS", "JNS", "JP", "JNP", "JL", "JNL", "JLE", "JNLE",
        "ADD", "ADD", "SUB", "SUB", "TEST","TEST","XCHG","XCHG","MOV","MOV","MOV","MOV","MOV","LEA","MOV", "POP",
        "NOP", "XCHG EAX, ECX", "XCHG EAX, EDX", "XCHG EAX, EBX", "XCHG EAX, ESP", "XCHG EAX, EBP", "XCHG EAX, ESI", "XCHG EAX, EDI", "CBW", "CWD", "CALL", "WAIT", "PUSHF", "POPF", "SAHF", "LAHF",
        "MOV AL,","MOV EAX","MOV X, AL", "MOV X, EAX", "MOVSB", "MOVSW", "CMPSB", "CMPSW", "TEST AL, ", "TEST EAX", "STOSB", "STOSW", "LODSB","LODSW", "SCASB", "SCASW",
        "MOV AL,", "MOV CL,", "MOV DL,", "MOV BL,", "MOV AH,","MOV CH,", "MOV DH, ", "MOV BH,", "MOV EAX,", "MOV ECX, ", "MOV EDX, ", "MOV EBX,", "MOV ESP,", "MOV EBP,", "MOV ESI,", "MOV EDI,",
        "N/A","N/A","RETN", "RETN", "LES", "LDS", "MOV","MOV","ENTER", "LEAVE", "RETF", "RETF", "INT3","INT", "INTO", "IRET",
        "N/A", "N/A", "N/A", "N/A", "AAM", "AAD", "SALC", "XLAT", "ESC", "ESC", "ESC", "ESC" , "ESC", "ESC", "ESC", "ESC",
        "LOOPNZ", "LOOPZ", "LOOP", "JCXZ", "IN AL", "IN EAX", "OUT AL", "OUT EAX", "CALL", "JMP", "JMP", "JMP", "IN AL, DX", "IN EAX, DX", "OUT DX, AL", "OUT DX, EAX",
        "LOCK", "INT1", "REPNE:", "REP:", "HLT", "CMC", "N/A", "N/A", "CLC", "STC", "CLI", "STI", "CLD", "STD", "N/A", "N/A"
    };
    set_color(0x1f);
    clear_screen();
    kprintf("f0und: Kernel panic!\nMessage: I'm sorry, your computer ran into an exception while running. Please be patient while we attempt to fetch information about what went wrong.\n");
    kprintf("Exception: %s Exception!\n", exceptions[regs->kernel.int_no & 0xff]);
    padding = 8;
    kprintf("Occured at:    %x\n", regs->kernel.eip);
    kprintf("Base Pointer:  %x\n", regs->kernel.ebp);
    kprintf("Stack Pointer: %x\n", regs->kernel.esp);
    kprintf("Opcode: %x (%s)| CS: %x | DS: %x | SS: %x\n", *((unsigned int *)(long)regs->kernel.eip), opcodes[*((unsigned char *)(long)regs->kernel.eip)], regs->kernel.cs, regs->kernel.ds, regs->kernel.ss == 0x10);
    kprintf("When you are ready, please restart your computer to continue. Any data from before the exception unfortuantely may be lost.\n");
    kprintf("f0und: End Kernel Panic! Result: Critical Exception. Restart.\nCode: %x%x\n", regs->kernel.int_no, regs->kernel.err_code);
    kprintf("Uptime: %ds", seconds);
    for(;;);
}


extern void _fhandler(irq_registers_t *regs, ...){
    char *exceptions[] = {
        "Divide by 0",
        "Reserved",
        "NMI Interrupt",
        "Breakpoint",
        "Overflow",
        "Bounds range exceeded",
        "Invalid Opcode",
        "Device not Available",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Invalid TSS",
        "Segment not Present",
        "Stack-segment",
        "General Protection",
        "Page Fault",
        "Reserved",
        "x87 FPU error",
        "Alignment Check",
        "Machine Check",
        "SIMD FP",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Software Panic!",
    };
    kernel_mode = 1;
    // if( active_procs < 2){
    //     panic(regs);
    // }
    panic(regs);
    kprintf("%s (PID: %x)\n",exceptions[regs->user.int_no], get_pid());
    exit_proc(-1, get_pid());
    schedule(regs);
    kernel_mode = 0;
}

idt_descriptor idt_desc;

void *_irq_handlers[256] = {0};

int irq_install_handler(void *handler, int irq_number){
    if(_irq_handlers[irq_number+ 0x20]){
        return -1;
    }
    _irq_handlers[irq_number + 0x20] = handler;
    return 0;
}
int int_install_handler(void *handler, int int_num){
    if(_irq_handlers[int_num]){
        return -1;
    }
    _irq_handlers[int_num] = handler;
    return 0;
}
void irq_remap(){
    outb(0x20, 0x11 );
    outb(0xa0, 0x11);
    outb(0x21, 0x20);
    outb(0xa1, 0x28);
    outb(0x21, 4);
    outb(0xa1, 2);
    outb(0x21, 1);
    outb(0xa1, 1);
    outb(0x21, 0);
    outb(0xa1, 0);
}

//i would never do thing normally but im using the int to long typecasts to get rid of the warnings because
//THE STUPID THING DOESNT UNDERSTAND WHAT IM TRYING TO CODE >:( I DONT WANT WARNINGS HIGHLIGHTED, JUST ERRORS >:(((

/*
TODO: Change IO operations to start using the job queue
*/
void native_irq(irq_user_registers_t *regs){
    switch(regs->eax){
        case 0:
        //sys_exit
            exit_proc(regs->ebx, get_pid());
            break;
        case 1:
        //sys_fopen()
        //TODO: update to work with new VFS layer with mount points instead of indexes
            if(((char *)(long)(regs->ebx))[1] == ':' && ((char *)(long)(regs->ebx))[2] == '/'){
                regs->eax = (uint32_t)(long)fopen((void *)(long)regs->ebx, regs->ecx);
            }
            else{
                char *str = (void *)(long)regs->ebx;
                kstrcat(str, get_proc(get_pid())->path_to_exec, str);
                regs->eax = (uint32_t)(long)fopen(str, regs->ecx);
            }
            break;
        case 2:
        //sys_read
        //returns regs->edx amount of characters.
        //is null terminated
        //returns amount of chars actually read in edx
            if(regs->ebx == 0){
                proc_list_t *proc = get_proc(get_pid());
                uint32_t stdinlen = kstrlen(proc->process->stdin);
                regs->edx = stdinlen * (stdinlen < regs->edx) + regs->edx * (stdinlen >= regs->edx);
                if(regs->ecx < 0x80000){
                    exit_proc(-1, get_pid());
                }
                kmemcpy(proc->process->stdin, (void *)(long)proc->process->regs.user.ecx, regs->edx);
                kmemcpy(proc->process->stdin + regs->edx, proc->process->stdin, kstrlen(proc->process->stdin) - regs->edx);
                proc->process->stdin[kstrlen(proc->process->stdin)] = 0;
            }
            else{
                fread((FILE *)(long)regs->ebx, (uint8_t *)(long)regs->ecx, regs->edx);
            }
            break;
        case 3:
        //sys_write
            if(regs->ebx == 0){
                proc_list_t *proc = get_proc(get_pid());
                kstrcat(proc->process->stdout, proc->process->stdout, (char *)(long)regs->ecx);
            }
            else {
                fwrite((FILE *)(long)regs->ebx, (uint8_t *)(long)regs->ecx, regs->edx);
            }
            break;
        case 4:
        //sys_close
            //TODO: Implement closing function, + locks, mutexes etc
            kfree((void *)(long)regs->ebx);
            break;
        case 5:
        //sys_fork
            fork(get_proc(get_pid()));
            break;
        case 6:
        //sys_exec
            // if(!load_exe){
                // regs->eax = -1;
            // }
            // if(load_exe((FILE *)(long)regs->ebx, regs->ecx, (void *)(long)regs->edx)) regs->eax = -1;
            
            break;
        case 7:
        //sys_get_pid
            regs->eax = get_pid();
            break;
        case 8:
        //sys_get_proc
        //yes im aware of the security implications...
            regs->eax = (uint32_t)(long)get_proc(regs->ebx);
            break;
        case 9:
        //sys_malloc
            regs->eax = (uint32_t)(long)kumalloc(regs->ebx/4096 + 1, 6, get_pid());
            break;
        case 10:
        //sys_free
            kfree((void *)(long)regs->ebx);
            break;
        
    }
}

void software_int(irq_user_registers_t* regs){
    uint16_t pid = get_pid();
    if(get_proc(pid)->process->abi == 0){
        native_irq(regs);
    }
}


extern void _irq_handler(irq_registers_t *regs){
    kernel_mode = 1;
    // kprintf("EAX: %x EBX: %x ECX: %x EDX: %x \nEBP: %x ESP: %x TOP OF STACK: %x\nEIP: %x DS: %x CS: %x SS:%x",
    // regs->eax, regs->ebx, regs->ecx, regs->edx, regs->ebp, regs->esp, *((uint32_t *)(long)regs->esp), regs->eip, regs->ds, regs->cs, regs->ss);
    irq_registers_t*(*handler)(irq_registers_t*r);
    if(get_proc(get_pid())->flags.ring0 && (unsigned char) regs->user.int_no == 0x80){
        software_int(&regs->user);
        // kprintf("int test");
        regs = schedule(regs);
        kernel_mode = 0;
        return;
    }
    handler = (irq_registers_t*(*)(irq_registers_t*))_irq_handlers[(unsigned char)regs->kernel.int_no];

    if(handler){
        regs = handler(regs);
    }
    else{
        padding = 0;
        kprintf("f0und: Error! No IRQ handler installed for IRQ %d!\n", regs->kernel.int_no);
    }

    if(regs->kernel.int_no >= 0x28 && regs->kernel.int_no < 0x30){
        outb(0xa0, 0x20);
    }
    if(regs->kernel.int_no < 0x30){
        outb(0x20, 0x20);
    }
    kernel_mode = 0;
    // kprintf("\nEAX: %x EBX: %x ECX: %x EDX: %x \nEBP: %x ESP: %x TOP OF STACK: %x\nEIP: %x DS: %x CS: %x SS:%x\nEFLAGS: %x\n",
    // regs->eax, regs->ebx, regs->ecx, regs->edx, regs->ebp, regs->esp, *((uint32_t *)(long)regs->esp), regs->eip, regs->ds, regs->cs, regs->ss, regs->eflags);
}
pic_mask_master_t master_pic_mask;
pic_mask_slave_t slave_pic_mask;

void init_idt(){
    // padding = 8;
    struct idt_flags_s isr_flags = (struct idt_flags_s){0xe, 0, 0, 1};
    for(int i = 0; i < 32; i++){
        idt_set(i, ( (long)_isr0 + i * (_isr1 - _isr0)), isr_flags);
    }
    // idt_set(0, _isr0(), )
    for(int i = 0; i < 16; i++){
        idt_set(i + 32, (long)irq0 + i * (irq1 - irq0), isr_flags);
    }
    // idt_desc.size = (long)&idt_entries[255] - (long)&idt_entries;
    // idt_desc.offset = (long)&idt_entries;
    // padding = 8;
    // pic_mask_master.byte = 0;
    // pic_mask_slave.byte = 0;
    idt_desc.offset = (unsigned long)&idt_entries;
    idt_desc.size = ((unsigned short)sizeof(idt_descriptor) * 256) - 1;
    irq_remap();
    idt_set(0x80, (unsigned long)softint, isr_flags);
    
    outb(0x21, 0xff);
    outb(0xa1, 0xff);
    slave_pic_mask.byte = 0xff;
    master_pic_mask.byte = 0xff;
    asm("sti");
    load_idt();
    int_install_handler(softint, 0x80);
    return;
}


void pic_remask(){
    outb(0x21, master_pic_mask.byte);
    outb(0xa1, slave_pic_mask.byte);
}