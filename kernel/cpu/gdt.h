#pragma once
#include <cstdint>
#include <stdint.h>
//WOW! i'm actually improving my GDT code!

typedef struct gdt_entry32_s{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    struct access_byte32_s{
        union gdt_access_low_u{
            struct gdt_data_code_desc_s{
                //leave this zero
                uint8_t accessed:1;
                //For code, if clear, cannot read from this segment, only execute.
                //For data, if clear cannot write to this segment.
                uint8_t read_write:1;
                //Direction is self explanatory. If conforming (code segment), only code from this or lower privlege level can execute code here. Otherwise, only code from equal privledge
                uint8_t direction_conforming:1;
                //if 1, code segment. If zero, data segment
                uint8_t executable:1;
            }data_code_desc;
            uint8_t type:4;
        }low;
        //Descriptor: if 0, defines system segment
        uint8_t desc:1;
        //Privledge level
        uint8_t dpl : 2;
        //does it exist?
        uint8_t present:1;
    }__attribute__((packed))access;
    struct gdt_flags32_s{
        uint8_t limit_high:4;
        uint8_t res:1;
        //if 1, 64 bit code. if zero, 16 or 32 bit
        uint8_t long_mode:1;
        //if 0, limit is in bytes, if 1, effective_limit = limit * 4096
        uint8_t granularity:1;
    }flags;
    uint8_t base_high;
}__attribute__((packed)) gdt_entry32_t;