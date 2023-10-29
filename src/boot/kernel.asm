; Kernel boot file
; Copyright (C) 2023 Panagiotis

MBOOT_HEADER_MAGIC  equ 0xE85250D6
MBOOT_ARCH          equ 0x00000000
KERNEL_STACK_SIZE   equ 4096
global KERNEL_VIRTUAL_BASE
KERNEL_VIRTUAL_BASE equ 0xC0000000                  ; 3GB
KERNEL_PAGE_NUMBER  equ (KERNEL_VIRTUAL_BASE >> 22)  ; Page directory index of kernel's 4MB PTE.

; Legacy from multiboot 1
; MBOOT_PAGE_ALIGN    equ 1 << 0
; MBOOT_MEM_INFO      equ 1 << 1
; MBOOT_GRAPH_MODE    equ 1 << 2

; MBOOT_HEADER_FLAGS  equ MBOOT_ARCH | MBOOT_HEADER_LEN
; MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

bits    32

section .multiboot_header
header_start:
        dd  MBOOT_HEADER_MAGIC
        dd  MBOOT_ARCH
        dd  header_end - header_start
        dd  0x100000000 - (MBOOT_HEADER_MAGIC + MBOOT_ARCH + (header_end - header_start))
        
; best -> 1024x768x32
align 8
    dw 5
    dw 1
    dd 20
    dd 1024
    dd 768
    dd 32

align 8
    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 16
stack_bottom:
    resb 16384*4
stack_top:

section .boot
global start

extern kmain
start:
    ; init temporary paging
    mov eax, (initial_page_dir - 0xC0000000)
    mov cr3, eax

    mov ecx, cr4
    or ecx, 0x10
    mov cr4, ecx

    mov ecx, cr0
    or ecx, 0x80000000
    mov cr0, ecx

    jmp higher_half
    
Shutdown:
    mov ax, 0x1000
    mov ax, ss
    mov sp, 0xf000
    mov ax, 0x5307
    mov bx, 0x0001
    mov cx, 0x0003
    int 0x15

section .text
higher_half:
    mov esp, stack_top ; stack init

    add ebx, KERNEL_VIRTUAL_BASE ; make the address virtual
    ; add eax, KERNEL_VIRTUAL_BASE

    push ebx ; multiboot mem info pointer
    ; push eax

    call kmain

halt:
    hlt
    jmp halt

section .data
align 4096
global initial_page_dir 
initial_page_dir:
    dd 10000011b ; initial 4mb identity map, unmapped later

    times 768-1 dd 0 ; padding

    ; hh kernel start, map 16 mb
    dd (0 << 22) | 10000011b ; 0xC0000000
    dd (1 << 22) | 10000011b
    dd (2 << 22) | 10000011b
    dd (3 << 22) | 10000011b
    times 256-4 dd 0 ; padding

    ; dd initial_page_dir | 11b