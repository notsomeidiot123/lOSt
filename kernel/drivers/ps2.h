#pragma once
#include "../cpu/idt.h"
#include "kb.h"

extern char last_char;
extern char ps_2_interrupt_fired;

enum ControlChars{
    NUL,
    LCTRL = 0x80,
    LSHFT,
    RSHFT,
    LALT,
    CAPS,
    F1=0x90,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    NUMLK=0xa,
    SCRLK
};

extern int init_8042();
extern void ps2_handler(irq_registers_t *regs);
extern char getchar();

extern unsigned char get_lastkey();
extern void register_handlers(kb_handler translated_handler, kb_handler untranslated_handler);