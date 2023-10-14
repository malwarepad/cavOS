; Task assembly calls
; Copyright (C) 2023 Panagiotis

bits 32

extern current_task
extern tss

; void switch_context(Task* old, Task* new);
; pushes register state onto old tasks kernel stack (the one we're in right now) and
; pops the new state, making sure to update the kesps
global switch_context
switch_context:
    mov eax, [esp + 4] ; eax = old
    mov edx, [esp + 8] ; edx = new

    ; push registers that arent already saved by cdecl call etc.
    push ebp
    push ebx
    push esi
    push edi

    ; swap kernel stack pointer
    mov [eax + 4], esp ; old->kesp = esp
    mov esp, [edx + 4] ; esp = new->kesp

    pop edi
    pop esi
    pop ebx
    pop ebp

    ret ; new tasks change the return value using TaskReturnContext.eip