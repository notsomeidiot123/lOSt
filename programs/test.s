org 0x40000

mov eax, 4
mov ebx, 0
mov ecx, len
mov edx, msg
int 0x80

jmp $

msg: db "Hello, World!\n"
len equ $ - msg