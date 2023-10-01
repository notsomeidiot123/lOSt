# lOSt Kernel Default System Calls

## How to use

System calls are called by using the `int 0x80` instruction, with arguments passed through the registers.

Return values are always returned via the `eax` register.