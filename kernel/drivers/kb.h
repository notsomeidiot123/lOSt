#pragma once
#include <stdint.h>

typedef void (*kb_handler)(uint8_t c);

enum KB_ERRORS{
    KB_ERR_NULL,
    KB_ERR_NO_CHAR,
    KB_ERR_INVALID_FP,
    KB_ERR_DUAL_REG
};

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

extern void kb_listener(uint8_t key);
extern int register_kb_listener(void (*listener)(uint8_t keychar));
extern void request_register_driver(void (*driver_listener)(kb_handler));
