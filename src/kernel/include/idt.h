#ifndef IDT_H
#define IDT_H

#include "types.h"

/* Functions implemented in idt.c */
void set_idt_gate(int n, uint32_t handler, uint8_t flags);
void set_idt();
void clean_idt_entries();

#endif
