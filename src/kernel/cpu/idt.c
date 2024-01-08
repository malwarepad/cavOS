#include <idt.h>
#include <util.h>

// IDT Entry configurator
// Copyright (C) 2023 Panagiotis

#define KERNEL_CS 0x08

typedef struct {
  uint16_t low_offset;
  uint16_t sel;
  uint8_t  always0;
  uint8_t  flags;
  uint16_t high_offset;
} __attribute__((packed)) idt_gate_t;

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) idt_register_t;

#define IDT_ENTRIES 256
static idt_gate_t     idt[IDT_ENTRIES];
static idt_register_t idt_reg;

void clean_idt_entries() { memset(&idt, 0, sizeof(idt_gate_t) * 256); }

void set_idt_gate(int n, uint32_t handler, uint8_t flags) {
  idt[n].low_offset = low_16(handler);
  idt[n].sel = KERNEL_CS;
  idt[n].always0 = 0;
  idt[n].flags = flags;
  idt[n].high_offset = high_16(handler);
}

void set_idt() {
  idt_reg.base = (uint32_t)&idt;
  idt_reg.limit = IDT_ENTRIES * sizeof(idt_gate_t) - 1;
  __asm__ __volatile__("lidtl (%0)" : : "r"(&idt_reg));
}
