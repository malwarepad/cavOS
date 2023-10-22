#include "../../include/ata.h"
#include "../../include/backupconsole.h"
#include "../../include/console.h"
#include "../../include/disk.h"
#include "../../include/fat32.h"
#include "../../include/gdt.h"
#include "../../include/idt.h"
#include "../../include/isr.h"
#include "../../include/kb.h"
#include "../../include/liballoc.h"
#include "../../include/multiboot2.h"
#include "../../include/pci.h"
#include "../../include/pmm.h"
#include "../../include/rtc.h"
#include "../../include/serial.h"
#include "../../include/shell.h"
#include "../../include/task.h"
#include "../../include/testing.h"
#include "../../include/timer.h"
#include "../../include/util.h"
#include "../../include/vga.h"

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

  initiateSerial();
  debugf("Multiboot2 reached:\n{magic: %x, mbi addr: %lx, size: %x}\n", magic,
         addr, mbi_size);

  setup_gdt();
  isr_install();

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
    } else if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
      struct multiboot_tag_framebuffer *tagfb =
          (struct multiboot_tag_framebuffer *)tag;
      framebuffer = tagfb->common.framebuffer_addr;
      framebufferHeight = tagfb->common.framebuffer_height;
      framebufferWidth = tagfb->common.framebuffer_width;
      framebufferPitch = tagfb->common.framebuffer_pitch;
      debugf("%dx%d\n", framebufferWidth, framebufferHeight);
    }
  }

  initiateBitmap(mbi);

  debugf("====== DEBUGGING LOGS ======\n\n");

  initiatePCI();
  initiateTimer(1000);
  initiateFat32();

  setup_tasks();

  initiateConsole();

  testingInit();

  printf("=========================================\n");
  printf("==     Cave-Like Operating System      ==\n");
  printf("==      Copyright MalwarePad 2023      ==\n");
  printf("=========================================\n\n");

  launch_shell(0);
}
