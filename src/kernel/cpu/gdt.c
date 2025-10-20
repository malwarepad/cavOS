#include <gdt.h>
#include <stdint.h>
#include <util.h>

// GDT & TSS Entry configurator
// Copyright (C) 2024 Panagiotis

static GDTEntries gdt;
static GDTPtr     gdtr;
static TSSPtr     tss;

TSSPtr *tssPtr = &tss;

void gdt_set_entry(int idx, uint32_t base, uint32_t limit, uint8_t access,
                   uint8_t granularity) {
  gdt.descriptors[idx].base_low = (base & 0xffff);
  gdt.descriptors[idx].base_mid = (base >> 16) & 0xff;
  gdt.descriptors[idx].access = access;
  gdt.descriptors[idx].granularity =
      ((limit >> 16) & 0x0f) | (granularity & 0xf0);
  gdt.descriptors[idx].base_high = (base >> 24) & 0xff;
  gdt.descriptors[idx].limit = limit;
}

void gdt_load_tss(TSSPtr *tss) {
  size_t addr = (size_t)tss;

  gdt.tss.base_low = (uint16_t)addr;
  gdt.tss.base_mid = (uint8_t)(addr >> 16);
  gdt.tss.flags1 = 0b10001001;
  gdt.tss.flags2 = 0;
  gdt.tss.base_high = (uint8_t)(addr >> 24);
  gdt.tss.base_upper32 = (uint32_t)(addr >> 32);
  gdt.tss.reserved = 0;

  asm volatile("ltr %0" : : "rm"((uint16_t)0x58) : "memory");
}

void gdt_reload() {
  asm volatile("lgdt %0\n\t"
               "push $0x28\n\t"
               "lea 1f(%%rip), %%rax\n\t"
               "push %%rax\n\t"
               "lretq\n\t"
               "1:\n\t"
               "mov $0x30, %%eax\n\t"
               "mov %%eax, %%ds\n\t"
               "mov %%eax, %%es\n\t"
               "mov %%eax, %%fs\n\t"
               "mov %%eax, %%gs\n\t"
               "mov %%eax, %%ss\n\t"
               :
               : "m"(gdtr)
               : "rax", "memory");
}

void initiateGDT() {
  // Null descriptor. (0)
  gdt_set_entry(0, 0, 0, 0, 0);

  // Kernel code 16. (8)
  gdt_set_entry(1, 0, 0xffff, 0b10011010, 0);

  // Kernel data 16. (16)
  gdt_set_entry(2, 0, 0xffff, 0b10010010, 0);

  // Kernel code 32. (24)
  gdt_set_entry(3, 0, 0xffff, 0b10011010, 0b11001111);

  // Kernel data 32. (32)
  gdt_set_entry(4, 0, 0xffff, 0b10010010, 0b11001111);

  // Kernel code 64. (40)
  gdt_set_entry(5, 0, 0, 0b10011010, 0b00100000);

  // Kernel data 64. (48)
  gdt_set_entry(6, 0, 0, 0b10010010, 0);

  // SYSENTER
  gdt.descriptors[7] = (GDTEntry){0}; // (56)
  gdt.descriptors[8] = (GDTEntry){0}; // (64)

  // User code 64. (72)
  gdt_set_entry(10, 0, 0, 0b11111010, 0b00100000);

  // User data 64. (80)
  gdt_set_entry(9, 0, 0, 0b11110010, 0);

  // TSS. (88)
  gdt.tss.length = 104;
  gdt.tss.base_low = 0;
  gdt.tss.base_mid = 0;
  gdt.tss.flags1 = 0b10001001;
  gdt.tss.flags2 = 0;
  gdt.tss.base_high = 0;
  gdt.tss.base_upper32 = 0;
  gdt.tss.reserved = 0;

  gdtr.limit = sizeof(GDTEntries) - 1;
  gdtr.base = (uint64_t)&gdt;

  gdt_reload();

  memset(&tss, 0, sizeof(TSSPtr));
  gdt_load_tss(&tss);
}
