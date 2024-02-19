; GDT assembly calls
; Copyright (C) 2024 Panagiotis

bits    32

global asm_flush_gdt
asm_flush_gdt:
    mov eax, [esp + 4]
    lgdt [eax]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    jmp 0x08:.load_cs
.load_cs:
    ret

global asm_flush_tss
asm_flush_tss:
    mov ax, 0x28
    ltr ax
    ret