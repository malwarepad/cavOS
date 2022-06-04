; Kernel boot file
; Copyright (C) 2022 Panagiotis

MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_PAGE_ALIGN    equ 1 << 0
MBOOT_MEM_INFO      equ 1 << 1
MBOOT_GRAPH_MODE    equ 1 << 2
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_GRAPH_MODE
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

bits    32

section         .text
        align   32
        dd MBOOT_HEADER_MAGIC
        dd MBOOT_HEADER_FLAGS
        dd MBOOT_CHECKSUM
   
        dd 0 ; skip some flags
        dd 0
        dd 0
        dd 0
        dd 0
        
        dd 0 ; sets it to graphical mode
        dd 1280 ; sets the width
        dd 720 ; sets the height
        dd 32 ; sets the bits per pixel

        
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

section .bss
resb 8192
stack_space:

Shutdown:
    mov ax, 0x1000
    mov ax, ss
    mov sp, 0xf000
    mov ax, 0x5307
    mov bx, 0x0001
    mov cx, 0x0003
    int 0x15

WaitForEnter:
    mov ah, 0
    int 0x16
    cmp al, 0x0D
    jne WaitForEnter
    ret
