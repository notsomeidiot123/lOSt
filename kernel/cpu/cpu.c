#include <cpuid.h>
#include "cpu.h"
#include "../graphics/vga.h"

cpuid_t cpuid_info = { 0 };

void call_cpuid(){
    unsigned int eax;
    unsigned int ebx;
    unsigned int ecx;
    unsigned int edx;
    __get_cpuid(0, &eax, &ebx, &ecx, &edx);
    cpuid_info.vendor_string[0] = ebx;
    cpuid_info.vendor_string[1] = edx;
    cpuid_info.vendor_string[2] = ecx;
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);
    cpuid_info.ebx1.as_num = ebx;
    cpuid_info.edx1.as_num = edx;
    cpuid_info.ecx1.as_num = ecx;
    kprintf("CPU Vendor: %s\n", (char *) cpuid_info.vendor_string);
}