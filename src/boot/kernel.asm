bits    32

section         .text
        align   4
        dd      0x1BADB002
        dd      0x00
        dd      - (0x1BADB002+0x00)
   
        dd 0 ; skip some flags
        dd 0
        dd 0
        dd 0
        dd 0
        
        dd 0 ; sets it to graphical mode
        dd 320 ; sets the width
        dd 200 ; sets the height
        dd 32 ; sets the bits per pixel

        
global start
extern kmain            ; this function is gonna be located in our c code(kernel.c)

start:
        cli             ;clears the interrupts 
        ;Int 0x10

        ;mov eax, cr0 
        ;or al, 1       ; set PE (Protection Enable) bit in CR0 (Control Register 0)
        ;mov cr0, eax

        mov esp, stack_space
        push ebx
        push eax
        ;push ebx
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
