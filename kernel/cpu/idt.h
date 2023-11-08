#pragma once
#include <stdint.h>
extern void load_idt();
extern void enable_int();
extern void disable_int();

extern short loaded_code_segment;
extern short loaded_data_segment;

extern void load_code_segment(short segment);

typedef struct{
    short size;
    int offset;
}__attribute__((packed))idt_descriptor;

typedef union{
    unsigned char byte;
    struct{
        unsigned char pit: 1;
        unsigned char ps2kb: 1;
        unsigned char cascade: 1;
        unsigned char com2: 1;
        unsigned char com1: 1;
        unsigned char lpt2: 1;
        unsigned char floppy: 1;
        unsigned char lpt1: 1;
    }data;
}pic_mask_master_t;
typedef union{
    unsigned char byte;
    struct{
        unsigned char cmos: 1;
        unsigned char perif_0: 1;
        unsigned char perif_1: 1;
        unsigned char perif_2: 1;
        unsigned char ps2m: 1;
        unsigned char fpu: 1;
        unsigned char pata_p: 1;
        unsigned char pata_s: 1;
    }data;
}pic_mask_slave_t;

extern pic_mask_master_t master_pic_mask;
extern pic_mask_slave_t slave_pic_mask;

extern int irq_install_handler(void *handler, int irq_number);

typedef struct
{
    unsigned int gs, fs, es, ds;      /* pushed the segs last */
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
    unsigned int eip, cs, eflags, useresp, ss;   /* pushed by the processor automatically */ 
} __attribute__((packed))irq_user_registers_t;
typedef struct
{
    unsigned int gs, fs, es, ds;      /* pushed the segs last */
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  /* pushed by 'pusha' */
    unsigned int int_no, err_code;    /* our 'push byte #' and ecodes do this */
    unsigned int eip, cs, eflags, ss;   /* pushed by the processor automatically */ 
}__attribute__((packed)) irq_kernel_registers_t;

typedef union irq_registers_u{
    irq_user_registers_t user;
    irq_kernel_registers_t kernel;
}irq_registers_t;

extern void init_idt();

void pic_remask();
uint32_t get_eflags();