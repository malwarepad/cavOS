#include <paging.h>
#include <pmm.h>
#include <system.h>
#include <util.h>
#include <vmm.h>

// Physical memory space manager/allocator
// Copyright (C) 2024 Panagiotis

void initiatePMM() {
  DS_Bitmap *bitmap = &physical; // pointer to pmm bitmap (used later)
  bitmap->ready = false;         // for bitmap dependency of vmm

  physical.BitmapSizeInBlocks = DivRoundUp(mbi_memorySize, BLOCK_SIZE);
  physical.BitmapSizeInBytes = DivRoundUp(physical.BitmapSizeInBlocks, 8);

  // paging.c uses bitframe allocation, and we need to compute and give it some
  // temporary memory to work with
  uint32_t pageTablesRequired =
      DivRoundUp(DivRoundUp(physical.BitmapSizeInBytes, PAGE_SIZE), 1024);

  uint8_t                 found = 0;
  multiboot_memory_map_t *mmmt;

  for (int i = 0; i < memoryMapCnt; i++) {
    mmmt = memoryMap[i];
    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE &&
        (mmmt->len) >=
            (physical.BitmapSizeInBytes + pageTablesRequired * PAGE_SIZE)) {
      // debugf("[%x %x - %x %x] {%x}\n", mmmt->addr_low, mmmt->len_high,
      //        mmmt->len_low, mmmt->type);
      found = 1;
      break;
    }
  }

  if (!found) {
    debugf("[pmm] Not enough memory: required{%d} found{%d}!\n",
           physical.BitmapSizeInBytes, mbi_memorySize);
    panic();
    return 1;
  }

  registerTmpPageFrame(mmmt->addr);
  uint32_t pageframeStart = mmmt->addr;
  mmmt->addr += pageTablesRequired * 4096;

  uint32_t bitmapStart =
      0xD0000000 -
      (DivRoundUp(bitmap->BitmapSizeInBytes, PAGE_SIZE) * PAGE_SIZE);
  uint32_t bitmapStartPhys = mmmt->addr;

  physical.Bitmap = (uint32_t *)bitmapStart;

  uint32_t pagesRequired = DivRoundUp(bitmap->BitmapSizeInBytes, PAGE_SIZE);
  for (int i = 0; i < pagesRequired; i++) {
    debugf("[pmm] Memory mapping %d/%d: virt{%x} phys{%x}\n", i, pagesRequired,
           bitmapStart + i * PAGE_SIZE, (uint32_t)mmmt->addr + i * PAGE_SIZE);
    VirtualMap(bitmapStart + i * PAGE_SIZE,
               (uint32_t)mmmt->addr + i * PAGE_SIZE, 0);
  }

  // When I found memory region issues, used this for debugging
  // for (int i = 0; i < physical.BitmapSizeInBlocks; i++) {
  // BitmapSet(bitmap, i, true);
  // debugf("%d/%d ", i, physical.BitmapSizeInBlocks);
  // }
  // debugf("\n");

  memset(physical.Bitmap, 0xff, physical.BitmapSizeInBytes);

  // for (int i = 0; i < memoryMapCnt; i++) {
  //   mmmt = memoryMap[i];
  //   if (mmmt->addr > mbi_memorySize)
  //     continue;
  //   if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
  //     MarkBlocks(ToBlockRoundUp(mmmt->addr), mmmt->len / BLOCK_SIZE, 0);
  //   }
  // }

  // for (int i = 0; i < memoryMapCnt; i++) {
  //   mmmt = memoryMap[i];
  //   if (mmmt->addr > mbi_memorySize)
  //     continue;
  //   if (mmmt->type != MULTIBOOT_MEMORY_AVAILABLE)
  //     MarkBlocks(ToBlock(mmmt->addr), DivRoundUp(mmmt->len, BLOCK_SIZE), 1);
  // }

  uint64_t best_start = 0;
  uint64_t best_len = 0;
  for (int i = 0; i < memoryMapCnt; i++) {
    mmmt = memoryMap[i];
    if (mmmt->addr > mbi_memorySize)
      continue;
    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
      debugf("[mm] %x-%x\n", (uint32_t)mmmt->addr,
             (uint32_t)mmmt->addr + (uint32_t)mmmt->len);
    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE && mmmt->len > best_len) {
      best_start = mmmt->addr;
      best_len = mmmt->len;
    }
  }

  // todo: extend this to the whole physical memory space!
  MarkBlocks(bitmap, ToBlockRoundUp(bitmap, best_start), best_len / BLOCK_SIZE,
             0);

  for (int i = 0; i < memoryMapCnt; i++) {
    mmmt = memoryMap[i];
    if (mmmt->addr > mbi_memorySize)
      continue;
    if (mmmt->type != MULTIBOOT_MEMORY_AVAILABLE) {
      debugf("[mm] [x] %x-%x\n", (uint32_t)mmmt->addr,
             (uint32_t)mmmt->addr + (uint32_t)mmmt->len);
      MarkBlocks(bitmap, ToBlock(bitmap, mmmt->addr),
                 DivRoundUp(mmmt->len, BLOCK_SIZE), 1);
    }
  }

  MarkRegion(bitmap, (void *)bitmapStartPhys, physical.BitmapSizeInBytes, 1);
  MarkRegion(bitmap, (void *)pageframeStart, pageTablesRequired * 4096, 1);
  MarkRegion(&physical, (void *)0x100000, 4096 * 10, 1); // qemu hack xd

  // for (uint32_t base = (0x100000 / BLOCK_SIZE) / 8;
  //      base < ((0x100000 / BLOCK_SIZE) / 8) + 128; base++) {
  //   debugf("%02X ", physical.Bitmap[base]);
  // }
  // debugf("\n");

  debugf("[pmm] Bitmap initiated: bitmapStartPhys{0x%x}{%d} "
         "pageFrameStart{0x%x}{%d} size{%d}\n",
         bitmapStartPhys, physical.BitmapSizeInBytes, pageframeStart,
         pageTablesRequired * 4096, physical.BitmapSizeInBytes);

  // BitmapDumpBlocks(bitmap);
  bitmap->ready = true;
}
