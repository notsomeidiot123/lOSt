#pragma once 
#include "../libs/types.h"
extern void seed_ksecr(uint32_t gseed);
extern uint32_t gen_driver_key(uint8_t pid, uint32_t modifier);