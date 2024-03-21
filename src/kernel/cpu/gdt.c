#include <gdt.h>
#include <util.h>

// GDT & TSS Entry configurator
// Copyright (C) 2024 Panagiotis

static GDTEntries gdt;
static GDTPtr     gdtr;
static TSSPtr     tss;

TSSPtr *tssPtr = &tss;

void initiateGDT() {
  // Null descriptor. (0)
  gdt.descriptors[0].limit = 0;
  gdt.descriptors[0].base_low = 0;
  gdt.descriptors[0].base_mid = 0;
  gdt.descriptors[0].access = 0;
  gdt.descriptors[0].granularity = 0;
  gdt.descriptors[0].base_high = 0;

  // Kernel code 16. (8)
  gdt.descriptors[1].limit = 0xffff;
  gdt.descriptors[1].base_low = 0;
  gdt.descriptors[1].base_mid = 0;
  gdt.descriptors[1].access = 0b10011010;
  gdt.descriptors[1].granularity = 0b00000000;
  gdt.descriptors[1].base_high = 0;

  // Kernel data 16. (16)
  gdt.descriptors[2].limit = 0xffff;
  gdt.descriptors[2].base_low = 0;
  gdt.descriptors[2].base_mid = 0;
  gdt.descriptors[2].access = 0b10010010;
  gdt.descriptors[2].granularity = 0b00000000;
  gdt.descriptors[2].base_high = 0;

  // Kernel code 32. (24)
  gdt.descriptors[3].limit = 0xffff;
  gdt.descriptors[3].base_low = 0;
  gdt.descriptors[3].base_mid = 0;
  gdt.descriptors[3].access = 0b10011010;
  gdt.descriptors[3].granularity = 0b11001111;
  gdt.descriptors[3].base_high = 0;

  // Kernel data 32. (32)
  gdt.descriptors[4].limit = 0xffff;
  gdt.descriptors[4].base_low = 0;
  gdt.descriptors[4].base_mid = 0;
  gdt.descriptors[4].access = 0b10010010;
  gdt.descriptors[4].granularity = 0b11001111;
  gdt.descriptors[4].base_high = 0;

  // Kernel code 64. (40)
  gdt.descriptors[5].limit = 0;
  gdt.descriptors[5].base_low = 0;
  gdt.descriptors[5].base_mid = 0;
  gdt.descriptors[5].access = 0b10011010;
  gdt.descriptors[5].granularity = 0b00100000;
  gdt.descriptors[5].base_high = 0;

  // Kernel data 64. (48)
  gdt.descriptors[6].limit = 0;
  gdt.descriptors[6].base_low = 0;
  gdt.descriptors[6].base_mid = 0;
  gdt.descriptors[6].access = 0b10010010;
  gdt.descriptors[6].granularity = 0;
  gdt.descriptors[6].base_high = 0;

  // SYSENTER
  gdt.descriptors[7] = (GDTEntry){0}; // (56)
  gdt.descriptors[8] = (GDTEntry){0}; // (64)

  // User code 64. (72)
  gdt.descriptors[9].limit = 0;
  gdt.descriptors[9].base_low = 0;
  gdt.descriptors[9].base_mid = 0;
  gdt.descriptors[9].access = 0b11111010;
  gdt.descriptors[9].granularity = 0b00100000;
  gdt.descriptors[9].base_high = 0;

  // User data 64. (80)
  gdt.descriptors[10].limit = 0;
  gdt.descriptors[10].base_low = 0;
  gdt.descriptors[10].base_mid = 0;
  gdt.descriptors[10].access = 0b11110010;
  gdt.descriptors[10].granularity = 0;
  gdt.descriptors[10].base_high = 0;

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

void gdt_load_tss(struct tss *tss) {
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
