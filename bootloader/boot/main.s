    org 0x7c00
    bits 16
MMAP_PTR equ 0x8000
OS_LOC   equ 0x10000
OS_START equ 8
;EXPERIMENT WITH A TWO-PARTITION DESIGN: CODE PARTITON AND FS PARTITION.
;FS MAY CONTAIN A CUSTOM SHELL, SO ALWAYS CHECK THAT

;Order of tasks:
;set stack  :check:
;enable unreal mode :checl:
;enable a20 :check:
;get mmap :check:
;load kernel into memory :check:
;setup GDT :check:
;load GDTR :check:
;set segment registers :check:
;jump to kernel, which will finish boot process :check:

init:
    jmp 0:.setstack
    .setstack:
        mov ax, 0
        mov ss, ax
        mov sp, 0x7a00
        mov bp, sp
        ; push dx
    push ds
    mov ax, 0
    mov ds, ax
    mov [partition], si
    mov [boot_disk], dl
    pop ds
    mov [partition_segment], ds
    mov bx,  strtab.start
    call Strs.printr
    jmp unreal
    jmp $
tmp_gdt_info:
    dw tmp_gdt_end -tmp_gdt - 1
    dd tmp_gdt
tmp_gdt:
dd 0, 0
code_desc: db 0xff, 0xff, 0, 0, 0, 10011010b, 0, 0
data_desc: db 0xff, 0xff, 0, 0, 0, 010010010b, 11001111b, 0
tmp_gdt_end:
unreal:
    xor ax, ax
    mov ds, ax
    mov ss, ax
    push ds
    cli
    lgdt [tmp_gdt_info]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x8:.pmode
    .pmode:
        mov bx, 0x10
        mov ds, bx
        and al, 0xfe
        mov cr0, eax
        jmp 0:.back
    .back:
    pop ds
    sti
    ;now we're in unreal mode, we can check for a20
    xor edx, edx
check_a20:
    ;edx MUST BE PRESERVED
    cmp edx, 3
    je no_a20
    mov ebx, 0x112345
    mov eax, 0x012345
    mov word [esi], 0
    mov word [edi], 0
    mov word [edi], 0xaa55
    cmpsb
    jne load_part2
    call enable_a20
    jmp check_a20
no_a20:
    mov bx, strtab.no_a20
    call Strs.printr
load_part2:
    ; mov ah, 0xe
    ; mov al, 0x43
    ; int 0x10
    xor ecx, ecx
    xor edx, edx
    xor eax, eax
    xor ebx, ebx
    mov esi, [partition]
    ; jmp $
    mov ah, 0x2
    mov al, 7
    mov ch, [esi + 3]
    mov cl, [esi + 2]
    add cl, 1
    mov dh, [esi + 1]
    mov dl, [boot_disk]
    mov bx, 0x7e00
    int 0x13
    jmp part2_start
    jmp $
Strs:
    .printr:
        mov si, bx
        mov ah, 0xe
        .pl0:
            lodsb
            cmp al, 0
            je .pl0e
            int 0x10
            jmp .pl0
        .pl0e:
            ret
    jmp $

enable_a20:
    cmp edx, 0
    jmp .bios
    cmp edx, 1
    jmp .kbdc
    cmp edx, 2
    jmp .fa20
    .bios:
        mov ax, 0x2401
        int 0x15
        jb .ret
        cmp ah, 0
        jb .ret
    .kbdc:
        cli
        call .wait
        mov al, 0xAD
        out 0x64, al

        call .wait
        mov al, 0xd0
        out 0x64, al

        call .wait2
        in al, 0x60
        push eax
        
        call .wait
        pop eax
        or al, 2
        out 0x60, al
        call .wait
        mov al, 0xAE
        out 0x64, al
        sti
        ret
    .wait:
        in al, 0x64
        test al, 2
        jne .wait
        ret
    .wait2:
        in al,0x64
        test al,1
        jz .wait2
        ret
    .wait3:
        mov eax, 0x100000
        .w3l0:
            cmp eax, 0
            je .ret
            dec eax
            jmp .w3l0
    .fa20:
        in al, 0x92
        or al, 2
        out 0x92, al
        jmp .wait3
    .ret:
        inc edx
        ret
    jmp $
boot_disk: db 0;
partition: dw 0
partition_segment: dw 0

part_head: db 0
part_sector: db 0
part_cyl: db 0

strtab:
    .start:             db "Booting lOSt... Please be paitent!", 0xa, 0xd, 0
    .no_a20:            db "Cannot flip a20 line, Aborting", 0xa, 0xd, 0
    .need_e820          db "Required BIOS Function e820 not supported!", 0xa, 0xd, 0
times 508 - ($ - $$) db 0
end_boot: db 0x5a
requested_size: db 0x30
db 0x55, 0xaa
ME820_MAGIC EQU 0x534d4150;SMAP
ELF_MAGIC EQU 0x7f454c46
jmp $
part2_start:
    .mmap:
        call get_mmap
    ;read kernel
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 0x2
    mov al, [requested_size]
    mov ch, [esi + 3]
    mov cl, [esi + 2]
    add cl, 8;this bootloader is 4096 bytes long, 8 sectors. add 8 to get kentry.o
    mov dh, [esi + 1]
    mov dl, [boot_disk]
    int 0x13

    setup_gdt:
    xor eax, eax
    ; mov al, byte [es:bx]
    ; cmp al, 0xeb
    ; je mnext0
    xor ax, ax
    mov ds, ax
    mov ss, ax

    lgdt [GDT_DESC]
    mov eax, cr0
    or al, 1
    mov cr0, eax

    jmp 0x8:jmpto
    jmp $
mmap_type: dw 0
mmap_count: dw 0

;;;MUST BE DWORD-ALIGNED!!!
unreal_memcpy:
    push eax
    xor eax, eax
    mov ds, eax
    pop eax
    .l0:
        cmp ecx, 0
        je .ret
        mov dx, [ebx+ecx]
        mov [eax+ecx], dx
        ; add ebx, 2
        ; add eax, 2
        dec ecx
        jmp .l0
    .ret:
        jmp setup_gdt
get_mmap:
    clc
    mov ax, 0
    mov es, ax
    xor ebx, ebx
    mov di, MMAP_PTR + 4
    mov edx, ME820_MAGIC
    mov eax, 0xe820
    mov ecx, 24
    mov [es:di+20], dword 1
    int 0x15
    jc mnext0
    cmp eax, ME820_MAGIC
    jne mnext0

    mov edx, eax
    test ebx, ebx
    je mnext0
    jmp .ml0e
    .ml0:
        mov eax, 0xe820
        mov [es:di+20], dword 1
        mov ecx, 24
        int 0x15
        jc .finished
        mov edx, ME820_MAGIC
    .ml0e:
        jcxz .finished
        cmp cl, 20
        jbe .nothing
        test [es:di + 20], dword 1
        je .skip
    .nothing:
        mov ecx, [es:di + 8]
        or ecx, [es:di + 12]
        jz .skip
        inc word [mmap_count]
        add di, 24
    .skip:
        test ebx, ebx
        jne .ml0
    .finished:
        clc
        ret
mnext0:
    inc byte [mmap_type]
    mov bx, strtab.need_e820
    call Strs.printr
    jmp $

GDT_DESC:
    dw GDT_END - GDT_START - 1;
    dd GDT_START
GDT_START:
    .NULL:
        DD 0
        DD 0
    .CODE:
        dw 0xffff
        dw 0
        db 0
        db 0b10011010
        db 0b11001111;11001111
        db 0
    .DATA:
        dw 0xffff
        dw 0
        db 0
        db 0b10010010
        db 0b11001111
        db 0
GDT_END:
[bits 32]
jmpto:    
    cli
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    
    
    push word [mmap_type]
    push word [mmap_count]
    push MMAP_PTR
    jmp 0x10000
    ; jmp $
    ; db "TEST"
times 4096 - ($-$$) db 0