#include <gdt.h>
#include <idt.h>
#include <util.h>

// IDT Entry configurator
// Copyright (C) 2024 Panagiotis

#define IDT_ENTRIES 256
static idt_gate_t     idt[IDT_ENTRIES] = {0};
static idt_register_t idt_reg;

void set_idt_gate(int n, uint64_t handler, uint8_t flags) {
  idt[n].isr_low = (uint16_t)handler;
  idt[n].kernel_cs = GDT_KERNEL_CODE;
  idt[n].ist = 0;
  idt[n].reserved = 0;
  idt[n].attributes = flags;
  idt[n].isr_mid = (uint16_t)(handler >> 16);
  idt[n].isr_high = (uint32_t)(handler >> 32);
}

void set_idt() {
  idt_reg.base = (size_t)&idt;
  idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
  asm volatile("lidt %0" ::"m"(idt_reg) : "memory");
}
