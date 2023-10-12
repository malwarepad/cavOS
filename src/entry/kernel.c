#include "../../include/ata.h"
#include "../../include/disk.h"
#include "../../include/fat32.h"
#include "../../include/gdt.h"
#include "../../include/idt.h"
#include "../../include/isr.h"
#include "../../include/kb.h"
#include "../../include/liballoc.h"
#include "../../include/multiboot2.h"
#include "../../include/pmm.h"
#include "../../include/rtc.h"
#include "../../include/shell.h"
#include "../../include/testing.h"
#include "../../include/util.h"

#include <stdint.h>

// Kernel entry file
// Copyright (C) 2023 Panagiotis

string center = "                   ";

int kmain(uint32 magic, unsigned long addr) {
  if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
    printf("Invalid magic number: 0x%x\n", (unsigned)magic);
    asm("hlt");
  }

  if (addr & 7) {
    printf("Unaligned mbi: 0x%x\n", addr);
    asm("hlt");
  }

  mbi_size = *(unsigned *)addr;
  mbi_addr = addr;
  mbi = (struct multiboot_tag *)(addr + 8);

  debugf("Multiboot2 reached:\n{magic: %x, mbi addr: %lx, size: %x}\n", magic,
         addr, mbi_size);

  while (mbi->type != MULTIBOOT_TAG_TYPE_END) {
    if (mbi->type == MULTIBOOT_TAG_TYPE_BASIC_MEMINFO) {
      struct multiboot_tag_basic_meminfo *memInfoTag;
      memInfoTag = (struct multiboot_tag_basic_meminfo *)mbi;

      mbi_memorySize = (memInfoTag->mem_lower + memInfoTag->mem_upper) * 1024;
      mbi_memorySizeKb = memInfoTag->mem_lower + memInfoTag->mem_upper;
    }
    mbi = (struct multiboot_tag *)((multiboot_uint8_t *)mbi +
                                   ((mbi->size + 7) & ~7));
  }

  memoryMapCnt = 0;
  for (struct multiboot_tag *tag = (struct multiboot_tag *)(addr + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag +
                                      ((tag->size + 7) & ~7))) {
    if (tag->type == MULTIBOOT_TAG_TYPE_MMAP) {
      struct multiboot_tag_mmap *mmapTag = (struct multiboot_tag_mmap *)tag;

      for (multiboot_memory_map_t *entry = mmapTag->entries;
           (multiboot_uint8_t *)entry < (multiboot_uint8_t *)tag + tag->size;
           entry = (multiboot_memory_map_t *)((unsigned long)entry +
                                              mmapTag->entry_size)) {
        memoryMap[memoryMapCnt++] = entry;
        // debugf("%lx\n", entry->addr);
      }
    }
  }

  initiateBitmap(mbi);

  clearScreen();

  debugf("====== DEBUGGING LOGS ======\n\n");
  printf("%s=========================================%s\n", center, center);
  printf("%s==     Cave-Like Operating System      ==%s\n", center, center);
  printf("%s==      Copyright MalwarePad 2023      ==%s\n", center, center);
  printf("%s=========================================%s\n\n", center, center);

  printf("[+] GDT: Setting up...\n");
  setup_gdt();
  printf("[+] GDT: Setup completed successfully!\n");

  printf("[+] ISR/IRQ: Setting up...\n");
  isr_install();
  printf("[+] ISR/IRQ: Setup completed successfully!\n");

  // printf("[+] Timer: Setting up...\n");
  // initiateTimer(1000);
  // printf("[+] Timer: Setup completed successfully!\n");

  initiateFat32();

  testingInit();

  printf("\n");

  launch_shell(0);
}
