#ifndef IDT_H
#define IDT_H

#include "types.h"

typedef struct {
  uint16_t isr_low;
  uint16_t kernel_cs;
  uint8_t  ist;
  uint8_t  attributes;
  uint16_t isr_mid;
  uint32_t isr_high;
  uint32_t reserved;
} __attribute__((packed)) idt_gate_t;

typedef struct {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed)) idt_register_t;

void set_idt_gate(int n, uint64_t handler, uint8_t flags);
void set_idt();

#endif
