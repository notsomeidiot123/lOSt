bits 32
; db "lOSt"
; dd 0x10000 + start
; jmp start

extern kmain
global start
start:
    mov ax, 0x10
    mov ds, ax
    call kmain
jmp $