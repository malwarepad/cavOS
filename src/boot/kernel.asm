; Kernel boot file
; Copyright (C) 2023 Panagiotis

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

        dd 1 ; 0 -> graphical, 1 -> tty
        dd 1280 ; sets the width
        dd 720 ; sets the height
        dd 32 ; sets the bits per pixel


global start
extern kmain            ; this function is gonna be located in our c code(kernel.c)

global enablePaging

enablePaging:
; load page directory (eax has the address of the page directory) 
   mov eax, [esp+4]
   mov cr3, eax        

; enable 4MBpage
;	mov ebx, cr4           ; read current cr4 
;	or  ebx, 0x00000010    ; set PSE  - enable 4MB page
;	mov cr4, ebx           ; update cr4

; enable paging 
   mov ebx, cr0          ; read current cr0
   or  ebx, 0x80000000   ; set PG .  set pages as read-only for both userspace and supervisor, replace 0x80000000 above with 0x80010000, which also sets the WP bit.
   mov cr0, ebx          ; update cr0
   ret                   ; now paging is enabled

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