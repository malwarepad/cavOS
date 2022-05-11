#ifndef ISR_H
#define ISR_H

#include "types.h"
#include "multiboot.h"

/* ISRs reserved for CPU exceptions */
void isr0(multiboot_info_t *mbi);
void isr1(multiboot_info_t *mbi);
void isr2(multiboot_info_t *mbi);
void isr3(multiboot_info_t *mbi);
void isr4(multiboot_info_t *mbi);
void isr5(multiboot_info_t *mbi);
void isr6(multiboot_info_t *mbi);
void isr7(multiboot_info_t *mbi);
void isr8(multiboot_info_t *mbi);
void isr9(multiboot_info_t *mbi);
void isr10(multiboot_info_t *mbi);
void isr11(multiboot_info_t *mbi);
void isr12(multiboot_info_t *mbi);
void isr13(multiboot_info_t *mbi);
void isr14(multiboot_info_t *mbi);
void isr15(multiboot_info_t *mbi);
void isr16(multiboot_info_t *mbi);
void isr17(multiboot_info_t *mbi);
void isr18(multiboot_info_t *mbi);
void isr19(multiboot_info_t *mbi);
void isr20(multiboot_info_t *mbi);
void isr21(multiboot_info_t *mbi);
void isr22(multiboot_info_t *mbi);
void isr23(multiboot_info_t *mbi);
void isr24(multiboot_info_t *mbi);
void isr25(multiboot_info_t *mbi);
void isr26(multiboot_info_t *mbi);
void isr27(multiboot_info_t *mbi);
void isr28(multiboot_info_t *mbi);
void isr29(multiboot_info_t *mbi);
void isr30(multiboot_info_t *mbi);
void isr31(multiboot_info_t *mbi);

void _irq();

string exception_messages[32];

void isr_install(multiboot_info_t *mbi);

#endif
