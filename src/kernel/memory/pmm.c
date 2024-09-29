#include <bootloader.h>
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

  physical.BitmapSizeInBlocks = DivRoundUp(bootloader.mmTotal, BLOCK_SIZE);
  physical.BitmapSizeInBytes = DivRoundUp(physical.BitmapSizeInBlocks, 8);

  struct limine_memmap_entry *mm = 0;

  for (int i = 0; i < bootloader.mmEntryCnt; i++) {
    struct limine_memmap_entry *entry = bootloader.mmEntries[i];
    if (entry->type != LIMINE_MEMMAP_USABLE ||
        entry->length < physical.BitmapSizeInBytes)
      continue;
    mm = entry;
    break;
  }

  if (!mm) {
    debugf("[pmm] Not enough memory: required{%lx}!\n",
           physical.BitmapSizeInBytes);
    panic();
    return;
  }

  size_t bitmapStartPhys = mm->base;
  physical.Bitmap = (uint8_t *)(bitmapStartPhys + bootloader.hhdmOffset);

  memset(physical.Bitmap, 0xff, physical.BitmapSizeInBytes);
  for (int i = 0; i < bootloader.mmEntryCnt; i++) {
    struct limine_memmap_entry *entry = bootloader.mmEntries[i];
    if (entry->type == LIMINE_MEMMAP_USABLE)
      MarkRegion(bitmap, (void *)entry->base, entry->length, 0);
  }
  for (int i = 0; i < bootloader.mmEntryCnt; i++) {
    struct limine_memmap_entry *entry = bootloader.mmEntries[i];
    if (entry->type != LIMINE_MEMMAP_USABLE)
      MarkRegion(bitmap, (void *)entry->base, entry->length, 1);
  }

  MarkRegion(bitmap, (void *)bitmapStartPhys, physical.BitmapSizeInBytes, 1);

  debugf("[pmm] Bitmap initiated: bitmapStartPhys{0x%lx} size{%lx}\n",
         bitmapStartPhys, physical.BitmapSizeInBytes);

  // BitmapDumpBlocks(bitmap);
  bitmap->ready = true;
}

Spinlock LOCK_PMM = ATOMIC_FLAG_INIT;

size_t PhysicalAllocate(int pages) {
  spinlockAcquire(&LOCK_PMM);
  size_t phys = (size_t)BitmapAllocate(&physical, pages);
  spinlockRelease(&LOCK_PMM);

  if (!phys) {
    debugf("[vmm::alloc] Physical kernel memory ran out!\n");
    panic();
  }

  return phys;
}

void PhysicalFree(size_t ptr, int pages) {
  // maybe verify no double-frees are occuring..

  spinlockAcquire(&LOCK_PMM);
  MarkRegion(&physical, (void *)ptr, pages * BLOCK_SIZE, 0);
  spinlockRelease(&LOCK_PMM);
}
