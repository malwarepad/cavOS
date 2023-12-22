#include <ata.h>
#include <backupconsole.h>
#include <console.h>
#include <disk.h>
#include <fat32.h>
#include <gdt.h>
#include <idt.h>
#include <isr.h>
#include <kb.h>
#include <liballoc.h>
#include <multiboot2.h>
#include <nic_controller.h>
#include <paging.h>
#include <pci.h>
#include <pmm.h>
#include <rtc.h>
#include <serial.h>
#include <shell.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <testing.h>
#include <timer.h>
#include <util.h>
#include <vga.h>
#include <vmm.h>

// Kernel entry file
// Copyright (C) 2023 Panagiotis

char *center = "                   ";

int kmain(unsigned long addr) {
  if (addr & 7) {
    debugf("Unaligned mbi: 0x%x\n", addr);
    printf("Unaligned mbi: 0x%x\n", addr);
    panic();
  }

  mbi_size = *(unsigned *)addr;
  mbi_addr = addr;
  mbi = (struct multiboot_tag *)(addr + 8);

  initiateSerial();
  debugf("Multiboot2 reached:\n{mbi virtaddr: %lx, size: %x}\n", addr,
         mbi_size);

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
    }
  }

  initiateBitmap(mbi);
  MarkRegion(addr, mbi->size, false); // don't touch the god damn multiboot info
  MarkRegion(&kernel_start,
             ((uint32_t)&kernel_end - KERNEL_START) - (uint32_t)&kernel_start,
             false); // not my kernel
  MarkRegion((uint32_t)&stack_bottom - KERNEL_START, 16384 * 8,
             false); // not my kernel stack man

  initiatePaging();

  for (struct multiboot_tag *tag = (struct multiboot_tag *)(addr + 8);
       tag->type != MULTIBOOT_TAG_TYPE_END;
       tag = (struct multiboot_tag *)((multiboot_uint8_t *)tag +
                                      ((tag->size + 7) & ~7))) {
    if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
      struct multiboot_tag_framebuffer *tagfb =
          (struct multiboot_tag_framebuffer *)tag;
      // framebuffer = tagfb->common.framebuffer_addr;
      framebuffer = KERNEL_GFX;
      framebufferHeight = tagfb->common.framebuffer_height;
      framebufferWidth = tagfb->common.framebuffer_width;
      framebufferPitch = tagfb->common.framebuffer_pitch;
      debugf("%dx%d\n", framebufferWidth, framebufferHeight);
      uint32_t size_bytes = framebufferWidth * framebufferHeight * 4;
      uint32_t needed_page_count = size_bytes / PAGE_SIZE + 1;

      for (uint32_t i = 0; i < needed_page_count; i++) {
        uint32_t offset = i * PAGE_SIZE;
        VirtualMap(KERNEL_GFX + offset,
                   ((uint32_t)tagfb->common.framebuffer_addr) + offset, 0);
      }

      framebuffer_end = KERNEL_GFX + (needed_page_count + 1) * PAGE_SIZE;
      sysalloc_base = framebuffer_end; // tmp
    }
  }

  debugf("====== DEBUGGING LOGS ======\n\n");

  initiateNetworking();
  initiatePCI();
  initiateTimer(1000);
  initiateFat32(0, 0);

  initiateConsole();

  initiateSyscalls();
  initiateTasks();

  testingInit();

  printf("=========================================\n");
  printf("==     Cave-Like Operating System      ==\n");
  printf("==      Copyright MalwarePad 2023      ==\n");
  printf("=========================================\n\n");

  launch_shell(0);
}
