; ISR assembly calls
; Copyright (C) 2024 Panagiotis

bits    64

global asm_finalize_sched
asm_finalize_sched:
  ; rdi = switch stack pointer
  ; rsi = next pagedir
  ; rdx = old task pointer (for cleanup)

  mov rsp, rdi
  mov cr3, rsi

  ; cleanup (WILL check task state, dw)
  mov rdi, rdx
  extern taskKillCleanup
  call taskKillCleanup

  pop rbp
  ; mov ds, ebp
  mov es, ebp

  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop rbp
  pop rdi
  pop rsi
  pop rdx
  pop rcx
  pop rbx
  pop rax

  add rsp, 16      ; pop error code and interrupt number
  iretq            ; pops (CS, EIP, EFLAGS) and also (SS, ESP) if privilege change occurs

global syscall_entry
syscall_entry:
  push rsp

  ; mimic: interrupt stuff
  push qword 0
  push qword 0
  push qword 0
  push qword 0
  push qword 0

  push qword 0 ; error
  push qword 0 ; interrupt

  push rax
  push rbx
  push rcx
  push rdx
  push rsi
  push rdi
  push rbp
  push r8
  push r9
  push r10
  push r11
  push r12
  push r13
  push r14
  push r15

  mov rbp, ds
  push rbp

  mov rdi, rsp
  extern handle_syscall_tssrsp
  call handle_syscall_tssrsp
  mov rsp, rax

  mov rdi, rsp
  extern syscallHandler
  call syscallHandler
  
  pop rbp
  mov ds, ebp
  mov es, ebp

  pop r15
  pop r14
  pop r13
  pop r12
  pop r11
  pop r10
  pop r9
  pop r8
  pop rbp
  pop rdi
  pop rsi
  pop rdx
  pop rcx
  pop rbx
  pop rax

  add rsp, 16      ; pop error code and interrupt number
  add rsp, 40      ; pop other interrupt stuff

  pop rsp ; reset rsp

  o64 sysret

isr_common:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rbp, ds
    push rbp

    mov bx, 0x30
    mov ds, bx
    mov es, bx
    mov ss, bx
    ; mov fs, bx
    ; mov gs, bx

    mov rdi, rsp
    extern handle_tssrsp
    call handle_tssrsp
    mov rsp, rax

		mov rdi, rsp
    extern handle_interrupt
    call handle_interrupt

global asm_isr_exit
asm_isr_exit:
    pop rbp
    ; mov ds, ebp
    mov es, ebp

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16      ; pop error code and interrupt number
    iretq            ; pops (CS, EIP, EFLAGS) and also (SS, ESP) if privilege change occurs

; generate isr stubs that jump to isr_common, in order to get a consistent stack frame

%macro ISR_ERROR_CODE 1
global isr%1
isr%1:
		; error code is already pushed
    push %1   ; interrupt number
    jmp isr_common
%endmacro

%macro ISR_NO_ERROR_CODE 1
global isr%1
isr%1:
    push 0    ; dummy error code to align with TrapFrame
    push %1   ; interrupt number
    jmp isr_common
%endmacro

global isr255
isr255:
    push 0    ; dummy error code to align with TrapFrame
    push 255   ; interrupt number
    jmp isr_common

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

section .data
global asm_isr_redirect_table
asm_isr_redirect_table:
%assign i 0
%rep 48
  dq isr%+i
%assign i i+1
%endrep