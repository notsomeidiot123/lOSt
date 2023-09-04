#pragma once
#include <stdint.h>

typedef void (*kb_handler)(uint8_t c);

extern void ps2_kb_listener(uint8_t key);
extern void register_kb_listener(void (*listener)(uint8_t keychar));