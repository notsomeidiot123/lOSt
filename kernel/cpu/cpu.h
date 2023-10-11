#pragma once
#include <stdint.h>
typedef struct cpuid_info_s{
    uint32_t vendor_string[3];
    uint8_t zero;
    union{
        int as_num;
        struct ebx_cpuid_1{
            uint8_t brand;
            uint8_t cl_flush_sz;
            uint8_t addr_ids;
            uint8_t apic_id;
        }as_struct;
    }ebx1;
    union {
        int as_num;
        struct edx_cpuid_1{
            uint32_t fpu:1;
            uint32_t vme:1;
            uint32_t de:1;
            uint32_t pse:1;
            uint32_t tsc:1;
            uint32_t msr:1;
            uint32_t pae:1;
            uint32_t mce:1;
            uint32_t cx8:1;
            uint32_t mtrr:1;
            uint32_t sep:1;
            uint32_t mtrr1:1;
            uint32_t pge:1;
            uint32_t mca:1;
            uint32_t cmov:1;
            uint32_t pat:1;
            uint32_t pse_36:1;
            uint32_t psn:1;
            uint32_t clfsh:1;
            uint32_t nx:1;
            uint32_t ds:1;
            uint32_t mmx:1;
            uint32_t fxsr:1;
            uint32_t sse:1;
            uint32_t sse2:1;
            uint32_t ss:1;
            uint32_t htt:1;
            uint32_t tm:1;
            uint32_t ia64:1;
            uint32_t pbe:1;
        }as_struct;
    }edx1;
    union {
        int as_num;
        struct ecx_cpuid_1{
            uint32_t sse3:1;
            uint32_t pclmulqdq:1;
            uint32_t dtes64:1;
            uint32_t monitor:1;
            uint32_t ds_cpl:1;
            uint32_t vmx:1;
            uint32_t smx:1;
            uint32_t est:1;
            uint32_t tm2:1;
            uint32_t ssse3:1;
            uint32_t cnxt_id:1;
            uint32_t sdbg:1;
            uint32_t fma:1;
            uint32_t cx16:1;
            uint32_t xtpr:1;
            uint32_t pdcm:1;
            uint32_t res:1;
            uint32_t pcid:1;
            uint32_t dca:1;
            uint32_t sse41:1;
            uint32_t sse42:1;
            uint32_t x2apic:1;
            uint32_t movbe:1;
            uint32_t popcnt:1;
            uint32_t tsc_deadline:1;
            uint32_t aes_ni:1;
            uint32_t xsave:1;
            uint32_t osxsave:1;
            uint32_t avx:1;
            uint32_t f16c:1;
            uint32_t rdrnd:1;
            uint32_t hypervisor:1;
        }as_struct;
    }ecx1;
}cpuid_t;

extern void call_cpuid();
