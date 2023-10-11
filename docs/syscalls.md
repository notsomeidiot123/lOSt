# lOSt Kernel Default System Calls

## How to use

System calls are called by using the `int 0x80` instruction, with arguments passed through the registers.

Return values are always returned via the `eax` register.

Syscall Table (32 Bits)

| Function | eax | ebx | ecx | edx |
|----------|-----|-----|-----|-----|
| sys_exit |  0  | uint32_t exit_code | * | * |
| sys_fopen|  1  | char *file_name | uint8_t mode | * |
| sys_read |  2  | FILE *file | void *buffer | size_t bytes |
| sys_write|  3  | FILE *file | void *buffer | size_t bytes |
| sys_close|  4  | FILE *file | * | * |
| sys_fork |  5  |  *  |  *  |  *  |
| sys_exec |  6  | FILE *file | * | * |
| sys_gpid |  7  | * | * | * |
| sys_getp |  8  | uint32_t pid | * | * |
