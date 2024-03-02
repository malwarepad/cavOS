#include <ata.h>
#include <backupconsole.h>
#include <console.h>
#include <disk.h>
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
// Copyright (C) 2024 Panagiotis

char *center = "                   ";

int kmain(unsigned long addr) {
  systemCallOnProgress = false;
  systemDiskInit = false;
  if (addr & 7) {
    debugf("[boot] Unaligned mbi: 0x%x!\n", addr);
    panic();
  }

  mbi_size = *(unsigned *)addr;
  mbi_addr = addr;
  mbi = (struct multiboot_tag *)(addr + 8);

  initiateSerial();
  debugf("[boot] Multiboot2 reached: mbi_virtaddr{%x} size{%x}\n", (size_t)addr,
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
      }
    }
  }

  initiatePaging();
  initiatePMM(mbi);
  debugf("[pmm] Reserved region: base{%x} len{%x}\n", addr - KERNEL_START,
         mbi->size);
  MarkRegion(&physical, addr - KERNEL_START, mbi->size,
             true); // don't touch the god damn multiboot info
  debugf("[pmm] Reserved region: base{%x} len{%x}\n", &kernel_start,
         ((size_t)&kernel_end - KERNEL_START) - (size_t)&kernel_start);
  MarkRegion(&physical, &kernel_start,
             ((size_t)&kernel_end - KERNEL_START) - (size_t)&kernel_start,
             true); // not my kernel
  debugf("[pmm] Reserved region: base{%x} len{%x}\n",
         (size_t)&stack_bottom - KERNEL_START, 16384 * 8);
  MarkRegion(&physical, (size_t)&stack_bottom - KERNEL_START, 16384 * 8,
             true); // not my kernel stack man
  initiateVMM();

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
      debugf("[graphics] Resolution fixed: width{%d} height{%d} bpp{%d}\n",
             framebufferWidth, framebufferHeight,
             tagfb->common.framebuffer_bpp);
      uint32_t size_bytes = framebufferWidth * framebufferHeight * 4;
      uint32_t needed_page_count = size_bytes / PAGE_SIZE + 1;
      debugf("[graphics] Memory area required: addr{%x} size{%d}\n",
             (size_t)tagfb->common.framebuffer_addr,
             needed_page_count * PAGE_SIZE);

      for (uint32_t i = 0; i < needed_page_count; i++) {
        uint32_t offset = i * PAGE_SIZE;
        // debugf("[graphics] Memory mapped at: %x!\n", KERNEL_GFX + offset);
        VirtualMap(KERNEL_GFX + offset,
                   ((size_t)tagfb->common.framebuffer_addr) + offset, 0);
      }

      framebuffer_end = KERNEL_GFX + (needed_page_count + 1) * PAGE_SIZE;
      sysalloc_base = framebuffer_end; // tmp
    }
  }
  MarkRegion(&virtual, KERNEL_GFX, framebuffer_end - KERNEL_GFX, 1);

  drawClearScreen();
  preFSconsoleInit();

  debugf("\n====== REACHED SYSTEM ======\n");
  initiateTimer(1000);
  initiateNetworking();
  initiatePCI();
  firstMountPoint = 0;
  fsMount("/", CONNECTOR_ATAPIO, 0, 0);

  // OpenFile *fr = fsKernelOpen("/files/lorem.txt");
  // char     *a = (char *)malloc(1026);
  // while (1) {
  //   memset(a, 0, 1026);
  //   uint32_t bytes = fsRead(fr, a, 1024);
  //   // debugf("[f] %d\n", bytes);

  //   for (int i = 0; i < 1024; i++) {
  //     debugf("%c", a[i]);
  //   }
  //   debugf("\n\n");

  //   debugf("ptr: %d\n", fr->pointer);
  //   if (bytes == 0)
  //     break;
  // }

  // panic();
  initiateConsole();

  initiateSyscalls();
  initiateTasks();

  testingInit();

  printf("=========================================\n");
  printf("==     Cave-Like Operating System      ==\n");
  printf("==      Copyright MalwarePad 2024      ==\n");
  printf("=========================================\n\n");

  launch_shell(0);
  panic();
}
