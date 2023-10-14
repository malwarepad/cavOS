#include "../../include/gdt.h"

// GDT & TSS Entry configurator
// Copyright (C) 2023 Panagiotis

void set_gdt_entry(uint32_t num, uint32_t base, uint32_t limit, uint8_t access,
                   uint8_t flags) {
  gdt_entries[num].base_low = (base & 0xFFFF);
  gdt_entries[num].base_mid = (base >> 16) & 0xFF;
  gdt_entries[num].base_high = (base >> 24) & 0xFF;

  gdt_entries[num].limit_low = (limit & 0xFFFF);
  gdt_entries[num].granularity = (limit >> 16) & 0x0F;

  gdt_entries[num].granularity |= flags & 0xF0;
  gdt_entries[num].access = access;
}

void setup_gdt() {
  gdt_pointer.limit = NUM_GDT_ENTRIES * 8 - 1;
  gdt_pointer.base = (uint32_t)&gdt_entries;

  memset((uint8_t *)&tss, 0, sizeof(tss));
  tss.ss0 = GDT_KERNEL_DATA;

  set_gdt_entry(0, 0, 0, 0, 0);                // 0x00: null
  set_gdt_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xC0); // 0x08: kernel mode text
  set_gdt_entry(2, 0, 0xFFFFFFFF, 0x92, 0xC0); // 0x10: kernel mode data
  set_gdt_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xC0); // 0x18: user mode code segment
  set_gdt_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xC0); // 0x20: user mode data segment
  set_gdt_entry(5, (uint32_t)&tss, sizeof(tss), 0x89, 0x40); // 0x28: tss

  asm_flush_gdt((uint32_t)&gdt_pointer);
  asm_flush_tss();
}
