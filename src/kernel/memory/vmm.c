#include <paging.h>
#include <pmm.h>
#include <system.h>
#include <task.h>
#include <util.h>
#include <vmm.h>

// Virtual memory space manager/allocator
// Copyright (C) 2023 Panagiotis

#define VMM_DEBUG 0

void initiateVMM() {
  virtual.ready = false;
  virtual.mem_start = 0xC0000000;
  virtual.BitmapSizeInBlocks = DivRoundUp(KERNEL_SPACE, BLOCK_SIZE);
  virtual.BitmapSizeInBytes = DivRoundUp(virtual.BitmapSizeInBlocks, 8);
  uint32_t bitmaploc = 0xC6000000;
  // sysalloc_base += virtual.BitmapSizeInBytes;

  for (int i = 0; i < DivRoundUp(virtual.BitmapSizeInBytes, BLOCK_SIZE); i++) {
    uint32_t fr = BitmapAllocatePageframe(&physical);
    VirtualMap(bitmaploc + i * PAGE_SIZE, fr,
               0); // 101000 + i * PAGE_SIZE
    debugf("%d/%d (%x)\n", i, DivRoundUp(virtual.BitmapSizeInBytes, BLOCK_SIZE),
           fr);
  }

  virtual.Bitmap = (uint8_t *)bitmaploc;

  memset(virtual.Bitmap, 0, virtual.BitmapSizeInBytes);

  MarkRegion(&virtual, virtual.Bitmap, virtual.BitmapSizeInBytes, 1);
  MarkRegion(&virtual, 0xcfff8000, 0xD0000000 - 0xcfff8000, 1);
  MarkRegion(&virtual, 0xC0000000, 0x1000000, 1);
  MarkRegion(&virtual, 0xFFFFF000, 0xFFFFFFFF - 0xFFFFF000, 1);
  virtual.ready = true;
}

void *VirtualAllocate(int pages) {
  void *output = BitmapAllocate(&virtual, pages);
  if (!output) {
    debugf("[virtual//allocate] Virtual kernel memory ran out!\n");
    panic();
  }

  for (int i = 0; i < pages; i++) {
    uint32_t pageframe = BitmapAllocatePageframe(&physical);
    if (!pageframe) {
      debugf("[virtual//allocate] Physical kernel memory ran out!\n");
      panic();
    }
    VirtualMap(output + i * PAGE_SIZE, pageframe, 0);
  }

  return output;
}

PhysicallyContiguous VirtualAllocatePhysicallyContiguous(int pages) {
  PhysicallyContiguous out;
  void                *virt = BitmapAllocate(&virtual, pages);
  void                *phys = BitmapAllocate(&physical, pages);

  if (!virt) {
    debugf("[virtual//allocate//pc] Virtual kernel memory ran out!\n");
    panic();
  }

  if (!phys) {
    debugf("[virtual//allocate//pc] Physical kernel memory ran out!\n");
    panic();
  }

  for (int i = 0; i < pages; i++) {
    VirtualMap(virt + i * PAGE_SIZE, phys + i * PAGE_SIZE, 0);
  }

#if VMM_DEBUG
  debugf("[virtual//contiguous] virt: %x phys: %x\n", virt, phys);
#endif

  out.virt = virt;
  out.phys = phys;
  return out;
}

int VirtualFree(void *ptr, int pages) {
  for (int i = 0; i < pages; i++) {
    uint32   virtaddr = ptr + (i * PAGE_SIZE);
    uint32_t physaddr = (uint32_t)VirtualToPhysical(virtaddr);
#if VMM_DEBUG
    debugf("unmapping %x\n", physaddr);
#endif
    MarkRegion(&physical, physaddr, 1, 0);
  }
  MarkRegion(&virtual, ptr, pages * PAGE_SIZE, 0);
  // releaseInterrupts();
  return 0;
}
