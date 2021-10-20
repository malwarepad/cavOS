bits    32

section         .text
        align   4
        dd      0x1BADB002
        dd      0x00
        dd      - (0x1BADB002+0x00)
        
global start
extern kmain            ; this function is gonna be located in our c code(kernel.c)

start:
        cli             ;clears the interrupts 
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

WaitForEnter:
    mov ah, 0
    int 0x16
    cmp al, 0x0D
    jne WaitForEnter
    ret
