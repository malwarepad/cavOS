#include "../include/vmm.h"
#include "../include/paging.h"
#include "../include/pmm.h"

// Temporary virtual memory allocator
// todo: Make it bitmap-based like the pmm
// Copyright (C) 2023 Panagiotis

#define VMM_DEBUG 0

void *VirtualAllocate(int pages) {
  uint32_t base = sysalloc_base;
  sysalloc_base += pages * PAGE_SIZE;
  for (int i = 0; i < pages; i++) {
    uint32_t real = BitmapAllocatePageframe();
    VirtualMap(base + (i * PAGE_SIZE), real, 0);
#if VMM_DEBUG
    debugf("real:%x, fake:%x\n", real,
           VirtualToPhysical(base + (i * PAGE_SIZE)));
#endif
  }
  return (void *)base;
}

int VirtualFree(void *ptr, int pages) {
  for (int i = 0; i < pages; i++) {
    uint32   virtaddr = ptr + (i * PAGE_SIZE);
    uint32_t physaddr = (uint32_t)VirtualToPhysical(virtaddr);
#if VMM_DEBUG
    debugf("unmapping %x\n", physaddr);
#endif
    BitmapFreePageframe(physaddr);
    VirtualUnmap(virtaddr);
  }
  return 0;
}
