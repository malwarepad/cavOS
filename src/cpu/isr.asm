; ISR assembly calls
; Copyright (C) 2023 Panagiotis

bits    32

isr_common:
    ; push registers to match struct AsmPassedInterrupt (little endian)
    pushad
    push ds
    push es
    push fs
    push gs

    ; load kernel data segment
    push ebx
    mov bx, 0x10
    mov ds, bx
    mov es, bx
    mov fs, bx
    mov gs, bx
    pop ebx

    extern handle_interrupt
    call handle_interrupt

global asm_isr_exit
asm_isr_exit:           ; used for newly created tasks in order to skip having to build the entire return stack
    pop gs
    pop fs
    pop es
    pop ds
    popad
    add esp, 8      ; pop error code and interrupt number
    iret            ; pops (CS, EIP, EFLAGS) and also (SS, ESP) if privilege change occurs

; generate isr stubs that jump to isr_common, in order to get a consistent stack frame

%macro ISR_ERROR_CODE 1
global isr%1
isr%1:
    push dword %1   ; interrupt number
    jmp isr_common
%endmacro

%macro ISR_NO_ERROR_CODE 1
global isr%1
isr%1:
    push dword 0    ; dummy error code to align with TrapFrame
    push dword %1   ; interrupt number
    jmp isr_common
%endmacro

; exceptions and CPU reserved interrupts 0 - 31
ISR_NO_ERROR_CODE 0
ISR_NO_ERROR_CODE 1
ISR_NO_ERROR_CODE 2
ISR_NO_ERROR_CODE 3
ISR_NO_ERROR_CODE 4
ISR_NO_ERROR_CODE 5
ISR_NO_ERROR_CODE 6
ISR_NO_ERROR_CODE 7
ISR_ERROR_CODE    8
ISR_NO_ERROR_CODE 9
ISR_ERROR_CODE    10
ISR_ERROR_CODE    11
ISR_ERROR_CODE    12
ISR_ERROR_CODE    13
ISR_ERROR_CODE    14
ISR_NO_ERROR_CODE 15
ISR_NO_ERROR_CODE 16
ISR_NO_ERROR_CODE 17
ISR_NO_ERROR_CODE 18
ISR_NO_ERROR_CODE 19
ISR_NO_ERROR_CODE 20
ISR_NO_ERROR_CODE 21
ISR_NO_ERROR_CODE 22
ISR_NO_ERROR_CODE 23
ISR_NO_ERROR_CODE 24
ISR_NO_ERROR_CODE 25
ISR_NO_ERROR_CODE 26
ISR_NO_ERROR_CODE 27
ISR_NO_ERROR_CODE 28
ISR_NO_ERROR_CODE 29
ISR_NO_ERROR_CODE 30
ISR_NO_ERROR_CODE 31

; IRQs 0 - 15 are mapped to 32 - 47
ISR_NO_ERROR_CODE 32 ; PIT
ISR_NO_ERROR_CODE 33
ISR_NO_ERROR_CODE 34
ISR_NO_ERROR_CODE 35
ISR_NO_ERROR_CODE 36
ISR_NO_ERROR_CODE 37
ISR_NO_ERROR_CODE 38
ISR_NO_ERROR_CODE 39
ISR_NO_ERROR_CODE 40
ISR_NO_ERROR_CODE 41
ISR_NO_ERROR_CODE 42
ISR_NO_ERROR_CODE 43
ISR_NO_ERROR_CODE 44
ISR_NO_ERROR_CODE 45
ISR_NO_ERROR_CODE 46
ISR_NO_ERROR_CODE 47

; syscall 0x80
ISR_NO_ERROR_CODE 128
global isr128

global asm_isr_redirect_table
asm_isr_redirect_table:
%assign i 0
%rep 48
    dd isr%+i
%assign i i+1
%endrep