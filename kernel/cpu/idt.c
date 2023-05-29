#include "idt.h"
#include "io.h"
#include "../drivers/ps2.h"
#include "../graphics/vga.h"
#include "../drivers/serial.h"
#include "../drivers/ps2.h"

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
    set_color(0x1f);
    clear_screen();
    kprintf("f0und: Kernel panic!\nMessage: I'm sorry, your computer ran into an exception while running. Please   be patient while we attempt to fetch information about what went wrong.\n");
    kprintf("Exception: %s Exception!\n", exceptions[regs->int_no & 0xff]);
    padding = 8;
    kprintf("Occured at:    %x\n", regs->eip);
    kprintf("Base Pointer:  %x\n", regs->ebp);
    kprintf("Stack Pointer: %x\n", regs->esp);
    kprintf("Opcode: %x | CS: %x | DS: %x | SS: %x\n", *((unsigned int *)(long)regs->eip), regs->cs, regs->ds, regs->ss == 0x10);
    kprintf("When you are ready, please restart your computer to continue. Any data from before the exception unfortuantely may be lost.\n");
    kprintf("f0und: End Kernel Panic! Result: Critical Exception. Restart.\nCode: %x\n", regs->int_no | regs->err_code << 16);
    for(;;);
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

/*
INT TABLE FOR f0und (most syscalls based on linux)
eax=1,
exits process with error code ebx
eax=2,
forks current process, if non is active, starts a new process
eax=3,
puts ecx amount of characters from string edx to file descriptor ebx
eax=4,
reads ecx amount of characters from file descriptor ebx and stores at pointer edx



*/

//i would never do thing normally but im using the int to long typecasts to get rid of the warnings because
//THE STUPID THING DOESNT UNDERSTAND WHAT IM TRYING TO CODE >:( I DONT WANT WARNINGS HIGHLIGHTED, JUST ERRORS >:(((

void software_int(irq_registers_t* regs){
    switch(regs->eax){
        case 3:
            for(int i = 0; i < regs->ecx; i++){
                while(!ps_2_interrupt_fired);
                *(char *)((long)regs->edx + i) = getchar();
            }
        case 4:
            for(int i = 0; i < regs->ecx; i++){
                kputc(*(char *)((long)regs->edx + i));
            }
        break;
        default:
        break;
    }
}


extern void _irq_handler(irq_registers_t *regs){
    void (*handler)(irq_registers_t *r);

    handler = (void (*)(irq_registers_t*))_irq_handlers[regs->int_no];

    if(handler){
        handler(regs);
    }
    else{
        padding = 0;
        kprintf("f0und: Error! No IRQ handler installed for IRQ %d!\n", regs->int_no - 0x20);
    }

    if(regs->int_no >= 0x28 && regs->int_no < 0x30){
        outb(0xa0, 0x20);
    }
    if(regs-> int_no < 0x30){
        outb(0x20, 0x20);
    }
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