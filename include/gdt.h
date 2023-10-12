#include "../include/ata.h"
#include "../src/boot/asm_ports/asm_ports.h"
#include "types.h"

#ifndef GDT_H
#define GDT_H

typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t  base_mid;
  uint8_t  access;
  uint8_t  granularity;
  uint8_t  base_high;
} __attribute__((packed)) GDTEntry;

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) GDTPointer;

typedef struct {
  uint16_t previous_task, __previous_task_unused;
  uint32_t esp0;
  uint16_t ss0, __ss0_unused;
  uint32_t esp1;
  uint16_t ss1, __ss1_unused;
  uint32_t esp2;
  uint16_t ss2, __ss2_unused;
  uint32_t cr3;
  uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
  uint16_t es, __es_unused;
  uint16_t cs, __cs_unused;
  uint16_t ss, __ss_unused;
  uint16_t ds, __ds_unused;
  uint16_t fs, __fs_unused;
  uint16_t gs, __gs_unused;
  uint16_t ldt_selector, __ldt_sel_unused;
  uint16_t debug_flag, io_map;
} __attribute__((packed)) TSS;

void setup_gdt();
void set_gdt_entry(uint32_t num, uint32_t base, uint32_t limit, uint8_t access,
                   uint8_t flags);

void asm_flush_gdt(uint32_t addr);
void asm_flush_tss();

#endif