; Kernel boot file
; Copyright (C) 2023 Panagiotis

MBOOT_HEADER_MAGIC  equ 0xE85250D6
MBOOT_ARCH          equ 0x00000000

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

section         .text
global start
extern kmain            ; this function is gonna be located in our c code(kernel.c)

start:
        cli             ;clears the interrupts

        mov esp, stack_space
        push ebx
        push eax
        call kmain      ;send processor to continue execution from the kamin funtion in c code
        call Shutdown
        hlt             ; halt the cpu(pause it from executing from this address

Shutdown:
    mov ax, 0x1000
    mov ax, ss
    mov sp, 0xf000
    mov ax, 0x5307
    mov bx, 0x0001
    mov cx, 0x0003
    int 0x15


section .bss
resb 8192
stack_space: