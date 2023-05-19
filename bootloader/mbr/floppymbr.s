start:
    org 0x600
    bits 16
reloc:
    mov ax, 0x800
    mov bp, ax
    mov sp, ax
    mov ax, 0
    mov si, 0x7c00
    mov ds, ax
    mov es, ax
    mov ss, ax
    push dx
    mov di, 0x600
    mov cx, 128
    repne movsd
    jmp 0:read_partitions
read_partitions:
    mov cx, 0x200
    mov di, 0x7c00
    mov eax, 0
    rep stosw
    mov si, part_0
    .l0:
        mov al, [ds:si]
        xor ah, ah
        cmp ax, 0x80
        jge .boot
        cmp si, part_3
        jge no_bootable
        add si, 0x10
        jmp .l0
    .boot:
        ; sub si, 0x10
        mov ch, [ds:si + 3]
        mov cl, [ds:si + 2]
        mov dh, [ds:si + 1]
        mov ax, 0 
        mov es, ax ;es=0
        mov bx, 0x7c00 ;0:0x7c00 (0x00007c00)
        mov ah, 0x2 ;read from drive
        mov al, 1
        int 0x13
        cmp word [0x7c00 + 510], 0xaa55
        jne boot_error
        jmp 0:0x7c00
        jmp boot_error
    jmp $

no_bootable:
    mov ax, 0
    mov ds, ax
    mov si, strings.no_bootable
    mov ah, 0xe
    .l0:
        lodsb
        cmp al, 0
        je .l0f
        int 0x10
        jmp .l0
    .l0f:
    jmp $
boot_error:
    mov ax, 0
    mov ds, ax
    mov si, strings.boot_error
    mov ah, 0xe
    .l0:
        lodsb
        cmp al, 0
        je .l0f
        int 0x10
        jmp .l0
    .l0f:
    jmp $
strings:
    .no_bootable:   db "lOStMBR:", 0xa, "Error: No bootable partitions!", 0xa, 0xd, 0
    .boot_error:    db "lOStMBR:", 0xa, "Error: lOStMBR ran into an error while booting!", 0xa, 0xd, 0x0
times 440 - ($-$$) db 0
db "lOSt"
dw 0
part_0:
    .flags:
        db 0
    .start_head:
        db 0
    .start_sector:
        db 0
    .start_cyl:
        db 0
    .sys_id:
        db 0 ;0x7f - custom OS
    .end_head:
        db 0
    .end_sector:
        db 0
    .end_cyl:
        db 0
    .start_LBA:
        dd 0
    .size:
        dd 0
part_1: times 16 db 0
part_2: times 16 db 0
part_3: times 16 db 0
end_sector_one:
    times 510 - ($ - $$) db 0
    db 0x55, 0xaa