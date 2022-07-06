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


; uint8 _cdecl x86_Disk_Read(uint8_t drive,uint16_t cylinder,uint16_t sector,uint16_t head,uint8_t count, uint8 far * dataOut);

; uint8 _cdecl x86_Disk_GetDriveParams(uint8_t drive,uint8_t* driveTypeOut,uint16_t* cylindersOut,uint16_t* sectorsOut,uint16_t* headsOut);

global x86_Disk_Reset
x86_Disk_Reset:

    ; make new call frame
    push bp             ; save old call frame
    mov bp, sp          ; initialize new call frame

    mov ah, 0
    mov dl, [bp + 4]    ; dl - drive
    stc
    int 13h

    mov ax, 1
    sbb ax, 0           ; 1 on success, 0 on fail

    ; restore old call frame
    mov sp, bp
    pop bp
    ret

global x86_Disk_Read
x86_Disk_Read:

    ; make new call frame
    push bp             ; save old call frame
    mov bp, sp          ; initialize new call frame

    ; save modified regs
    push bx
    push es

    ; setup args
    mov dl, [bp + 4]    ; dl - drive

    mov ch, [bp + 6]    ; ch - cylinder (lower 8 bits)
    mov cl, [bp + 7]    ; cl - cylinder to bits 6-7
    shl cl, 6

    mov al, [bp + 8]    ; cl - sector to bits 0-5
    and al, 3Fh
    or cl, al

    mov dh, [bp + 10]   ; dh - head

    mov al, [bp + 12]   ; al - count

    mov bx, [bp + 16]   ; es:bx - far pointer to data out
    mov es, bx
    mov bx, [bp + 14]

    ; call int13h
    mov ah, 02h
    stc
    int 13h

    ; set return value
    mov ax, 1
    sbb ax, 0           ; 1 on success, 0 on fail

    ; restore regs
    pop es
    pop bx

    ; restore old call frame
    mov sp, bp
    pop bp
    ret

global x86_Disk_GetDriveParams
x86_Disk_GetDriveParams:

    ; make new call frame
    push bp             ; save old call frame
    mov bp, sp          ; initialize new call frame

    ; save regs
    push es
    push bx
    push si
    push di

    ; call int13h
    mov dl, [bp + 4]    ; dl - disk drive
    mov ah, 08h
    mov di, 0           ; es:di - 0000:0000
    mov es, di
    stc
    int 13h

    ; return
    mov ax, 1
    sbb ax, 0

    ; out params
    mov si, [bp + 6]    ; drive type from bl
    mov [si], bl

    mov bl, ch          ; cylinders - lower bits in ch
    mov bh, cl          ; cylinders - upper bits in cl (6-7)
    shr bh, 6
    mov si, [bp + 8]
    mov [si], bx

    xor ch, ch          ; sectors - lower 5 bits in cl
    and cl, 3Fh
    mov si, [bp + 10]
    mov [si], cx

    mov cl, dh          ; heads - dh
    mov si, [bp + 12]
    mov [si], cx

    ; restore regs
    pop di
    pop si
    pop bx
    pop es

    ; restore old call frame
    mov sp, bp
    pop bp
    ret

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
