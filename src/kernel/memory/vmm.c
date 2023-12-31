#include <paging.h>
#include <pmm.h>
#include <system.h>
#include <task.h>
#include <vmm.h>

// Temporary virtual memory allocator
// todo: Make it bitmap-based like the pmm
// Copyright (C) 2023 Panagiotis

#define VMM_DEBUG 0

void *VirtualAllocate(int pages) {
  lockInterrupts();
#if VMM_DEBUG
  debugf("ALLOCATING!\n");
#endif
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
  releaseInterrupts();
  return (void *)base;
}

void *VirtualAllocatePhysicallyContiguous(int pages) {
  lockInterrupts();
  void *physical = BitmapAllocate(pages);
  for (int i = 0; i < pages; i++) {
    VirtualMap(sysalloc_base + i * PAGE_SIZE, physical + i * PAGE_SIZE, 0);
  }
  memset((void *)sysalloc_base, 0, pages);
  void *target = (void *)sysalloc_base;
  sysalloc_base += pages * PAGE_SIZE;
  // for (int i = 0; i < 3; i++) {
  //   VirtualUnmap(sysalloc_base + i * PAGE_SIZE);
  // }
  // return physical;
  releaseInterrupts();
  return target;
}

int VirtualFree(void *ptr, int pages) {
  lockInterrupts();
  for (int i = 0; i < pages; i++) {
    uint32   virtaddr = ptr + (i * PAGE_SIZE);
    uint32_t physaddr = (uint32_t)VirtualToPhysical(virtaddr);
#if VMM_DEBUG
    debugf("unmapping %x\n", physaddr);
#endif
    BitmapFreePageframe(physaddr);
    VirtualUnmap(virtaddr);
  }
  releaseInterrupts();
  return 0;
}
