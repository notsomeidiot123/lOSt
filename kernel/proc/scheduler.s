global push_args

push_args:
    ; cli ;re-enable when debugging
    push ebp
    mov ebp, esp

    push ebx
    push ecx
    push edx
    mov eax, [ebp + 8]
    mov ebx, [ebp + 12]
    mov edx, [ebp + 16]
    mov ecx, esp ; store esp
    mov esp, edx
    .loop:
        cmp ebx, 0
        je .return
        push dword [eax]
        add eax, 4
        dec ebx
    ; jmp $
        jmp .loop

    .return:
    push dword 0
    mov eax, esp
    ; jmp $
    mov esp, ecx
    pop edx
    pop ecx
    pop ebx
    pop ebp
    ; sti ;re-enable for debugging
    ret