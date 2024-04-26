#include <bitmap.h>
#include <bootloader.h>
#include <limine.h>
#include <malloc.h>
#include <paging.h>
#include <pmm.h>
#include <system.h>
#include <task.h>
#include <types.h>
#include <util.h>
#include <vmm.h>

// System-wide page table & directory management
// Copyright (C) 2024 Panagiotis

// A small note to myself: When mapping or doing other operations, there is a
// chance that the respective page layer (pml4, pdp, pd, pt, etc) is using
// full-size entries (by setting the appropriate flag)! Although I dislike using
// those, I should be keeping this in mind, in case I ever do otherwise. Limine
// HHDM regions shouldn't cause any issues, since I purposefully avoid mapping
// or modifying memory mappings close to said regions...

#define PAGING_DEBUG 0

#define HHDMoffset (bootloader.hhdmOffset)
uint64_t *globalPagedir = 0;

void initiatePaging() {
  globalPagedir =
      (size_t *)(BitmapAllocatePageframe(&physical) + bootloader.hhdmOffset);

  // debugf("phys{%lx} virt{%lx}\n", bootloader.kernelPhysBase,
  //        bootloader.kernelVirtBase);
  // debugf("hhdm{%lx}\n", bootloader.hhdmOffset);

  // So, I'll make a memory layout, EXACTLY like my link.ld... (send help)
  /*uint64_t textLen = (uint64_t)(&kernel_text_end) -
                     (uint64_t)(&kernel_text_start) + BLOCK_SIZE;
  VirtualMapRegionByLength(bootloader.kernelVirtBase, bootloader.kernelPhysBase,
                           textLen, 0);

  uint64_t rodataLen = (uint64_t)(&kernel_rodata_end) -
                       (uint64_t)(&kernel_rodata_start) + BLOCK_SIZE;
  VirtualMapRegionByLength(bootloader.kernelVirtBase + textLen,
                           bootloader.kernelPhysBase + textLen, rodataLen, 0);
  debugf("aparently!\n");

  uint64_t dataLen =
      ROUND_4KB((uint64_t)(&kernel_data_end) - (uint64_t)(&kernel_data_start));
  VirtualMapRegionByLength(bootloader.kernelVirtBase + textLen + rodataLen,
                           bootloader.kernelPhysBase + textLen + rodataLen,
                           dataLen, PF_RW);*/

  /*VirtualMapRegionByLength(bootloader.kernelVirtBase,
  bootloader.kernelPhysBase, (uint64_t)(&kernel_end) -
  (uint64_t)(&kernel_start), PF_RW);

  size_t bytesNeeded =
      bootloader.mmTotal > 0x100000000 ? bootloader.mmTotal : 0x100000000;
  size_t pagesNeeded = DivRoundUp(bytesNeeded, 0x200000);
  debugf("pages: %d\n", pagesNeeded);
  for (int i = 0; i < pagesNeeded; i++) {
    VirtualMap2MB(bootloader.hhdmOffset + i * 0x200000, i * 0x200000, PF_RW);
  }

  struct limine_memmap_entry *fbmm = 0;
  for (int i = 0; i < bootloader.mmEntryCnt; i++) {
    struct limine_memmap_entry *entry = bootloader.mmEntries[i];
    if (entry->type == LIMINE_MEMMAP_FRAMEBUFFER)
      fbmm = entry;
  }

  if (fbmm) {
    size_t framebufferPages = DivRoundUp(fbmm->length, PAGE_SIZE_LARGE);
    for (int i = 0; i < framebufferPages; i++)
      VirtualMap2MB(framebuffer + i * PAGE_SIZE_LARGE,
                    fbmm->base + i * PAGE_SIZE_LARGE, PF_RW);
  }
  VirtualSeek(DivRoundUp(bootloader.hhdmOffset, 0x40000000) * 0x40000000 +
              6 * 0x40000000);*/

  // sleep(85);
  // ChangePageDirectory(globalPagedir);

  // I will keep on using limine's for the time being
  uint64_t targ;
  asm volatile("movq %%cr3,%0" : "=r"(targ));
  globalPagedir = (uint64_t *)(targ + bootloader.hhdmOffset);
  // VirtualSeek(bootloader.hhdmOffset);
}

void VirtualMapRegionByLength(uint64_t virt_addr, uint64_t phys_addr,
                              uint64_t length, uint64_t flags) {
#if ELF_DEBUG
  debugf("[paging::map::region] virt{%lx} phys{%lx} len{%lx}\n", virt_addr,
         phys_addr, length);
#endif
  uint32_t pagesAmnt = DivRoundUp(length, PAGE_SIZE);
  for (int i = 0; i < pagesAmnt; i++) {
    uint64_t xvirt = virt_addr + i * PAGE_SIZE;
    uint64_t xphys = phys_addr + i * PAGE_SIZE;
    VirtualMap(xvirt, xphys, flags);
  }
}

// Will NOT check for the current task and update it's pagedir (on the struct)!
void ChangePageDirectoryUnsafe(uint64_t *pd) {
  uint64_t targ = VirtualToPhysical((size_t)pd);
  if (!targ) {
    debugf("[paging] Could not change to pd{%lx}!\n", pd);
    panic();
  }
  asm volatile("movq %0, %%cr3" ::"r"(targ));

  globalPagedir = pd;
}

void ChangePageDirectory(uint64_t *pd) {
  if (tasksInitiated)
    currentTask->pagedir = pd;
  ChangePageDirectoryUnsafe(pd);
}

uint64_t *GetPageDirectory() { return (uint64_t *)globalPagedir; }

void invalidate(uint64_t vaddr) { asm volatile("invlpg %0" ::"m"(vaddr)); }

size_t VirtAllocPhys() {
  size_t phys = BitmapAllocatePageframe(&physical);

  void *virt = (void *)(phys + HHDMoffset);
  memset(virt, 0, PAGE_SIZE);

  return phys;
}

void VirtualMap(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
  if (virt_addr % PAGE_SIZE) {
    debugf("[paging] Tried to map non-aligned address! virt{%lx} phys{%lx}\n",
           virt_addr, phys_addr);
    panic();
  }
  virt_addr = AMD64_MM_STRIPSX(virt_addr);

  uint32_t pml4_index = PML4E(virt_addr);
  uint32_t pdp_index = PDPTE(virt_addr);
  uint32_t pd_index = PDE(virt_addr);
  uint32_t pt_index = PTE(virt_addr);

  if (!(globalPagedir[pml4_index] & PF_PRESENT)) {
    size_t target = VirtAllocPhys();
    globalPagedir[pml4_index] = target | PF_PRESENT | PF_RW | flags;
  }
  size_t *pdp =
      (size_t *)(PTE_GET_ADDR(globalPagedir[pml4_index]) + HHDMoffset);

  if (!(pdp[pdp_index] & PF_PRESENT)) {
    size_t target = VirtAllocPhys();
    pdp[pdp_index] = target | PF_PRESENT | PF_RW | flags;
  }
  size_t *pd = (size_t *)(PTE_GET_ADDR(pdp[pdp_index]) + HHDMoffset);

  if (!(pd[pd_index] & PF_PRESENT)) {
    size_t target = VirtAllocPhys();
    pd[pd_index] = target | PF_PRESENT | PF_RW | flags;
  }
  size_t *pt = (size_t *)(PTE_GET_ADDR(pd[pd_index]) + HHDMoffset);

  if (pt[pt_index] & PF_PRESENT)
    debugf("[paging] Overwrite (without unmapping) WARN!\n");
  pt[pt_index] = (P_PHYS_ADDR(phys_addr)) | PF_PRESENT | flags; // | PF_RW

  invalidate(virt_addr);
#if ELF_DEBUG
  debugf("[paging] Mapped virt{%lx} to phys{%lx}\n", virt_addr, phys_addr);
#endif
}

void VirtualMap2MB(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
  if (virt_addr % PAGE_SIZE) {
    debugf("[paging] Tried to map non-aligned address! virt{%lx} phys{%lx}\n",
           virt_addr, phys_addr);
    panic();
  }
  virt_addr = AMD64_MM_STRIPSX(virt_addr);

  uint32_t pml4_index = PML4E(virt_addr);
  uint32_t pdp_index = PDPTE(virt_addr);
  uint32_t pd_index = PDE(virt_addr);
  // uint32_t pt_index = PTE(virt_addr); // unused

  if (!(globalPagedir[pml4_index] & PF_PRESENT)) {
    size_t target = VirtAllocPhys();
    globalPagedir[pml4_index] = target | PF_PRESENT | PF_RW;
  }
  size_t *pdp =
      (size_t *)(PTE_GET_ADDR(globalPagedir[pml4_index]) + HHDMoffset);

  if (!(pdp[pdp_index] & PF_PRESENT)) {
    size_t target = VirtAllocPhys();
    pdp[pdp_index] = target | PF_PRESENT | PF_RW;
  }
  size_t *pd = (size_t *)(PTE_GET_ADDR(pdp[pdp_index]) + HHDMoffset);

  if (pd[pd_index] & PF_PRESENT)
    debugf("[paging] Overwrite (without unmapping) WARN!\n");
  pd[pd_index] = (P_PHYS_ADDR(phys_addr)) | PF_PS | PF_PRESENT |
                 PTE_GET_FLAGS(flags); // | PF_RW

  invalidate(virt_addr);
}

size_t VirtualToPhysical(size_t virt_addr) {
  if (!globalPagedir)
    return 0;

  if (virt_addr >= HHDMoffset && virt_addr <= (HHDMoffset + bootloader.mmTotal))
    return virt_addr - HHDMoffset;

  size_t virt_addr_init = virt_addr;
  virt_addr &= ~0xFFF;

  virt_addr = AMD64_MM_STRIPSX(virt_addr);

  uint32_t pml4_index = PML4E(virt_addr);
  uint32_t pdp_index = PDPTE(virt_addr);
  uint32_t pd_index = PDE(virt_addr);
  uint32_t pt_index = PTE(virt_addr);

  if (!(globalPagedir[pml4_index] & PF_PRESENT))
    return 0;
  /*else if (globalPagedir[pml4_index] & PF_PRESENT &&
           globalPagedir[pml4_index] & PF_PS)
    return (void *)(PTE_GET_ADDR(globalPagedir[pml4_index] +
                                 (virt_addr & PAGE_MASK(12 + 9 + 9 + 9))));*/
  size_t *pdp =
      (size_t *)(PTE_GET_ADDR(globalPagedir[pml4_index]) + HHDMoffset);

  if (!(pdp[pdp_index] & PF_PRESENT))
    return 0;
  /*else if (pdp[pdp_index] & PF_PRESENT && pdp[pdp_index] & PF_PS)
    return (void *)(PTE_GET_ADDR(pdp[pdp_index] +
                                 (virt_addr & PAGE_MASK(12 + 9 + 9))));*/
  size_t *pd = (size_t *)(PTE_GET_ADDR(pdp[pdp_index]) + HHDMoffset);

  if (!(pd[pd_index] & PF_PRESENT))
    return 0;
  /*else if (pd[pd_index] & PF_PRESENT && pd[pd_index] & PF_PS)
    return (
        void *)(PTE_GET_ADDR(pd[pd_index] + (virt_addr & PAGE_MASK(12 + 9))));*/
  size_t *pt = (size_t *)(PTE_GET_ADDR(pd[pd_index]) + HHDMoffset);

  if (pt[pt_index] & PF_PRESENT)
    return (size_t)(PTE_GET_ADDR(pt[pt_index]) +
                    ((size_t)virt_addr_init & 0xFFF));

  return 0;
}

uint32_t VirtualUnmap(uint32_t virt_addr) {
  // not really used anywhere atm, sooooo idc
  return 0;
}

uint64_t *PageDirectoryAllocate() {
  if (!tasksInitiated) {
    debugf("[paging] FATAL! Tried to allocate pd without tasks initiated!\n");
    panic();
  }
  uint64_t *out = VirtualAllocate(1);

  memset(out, 0, PAGE_SIZE);

  uint64_t *model = taskGet(KERNEL_TASK_ID)->pagedir;
  for (int i = 0; i < 512; i++)
    out[i] = model[i];

  return out;
}

// todo: clear orphans after a whole page level is emptied!
// destroys any userland stuff on the page directory
void PageDirectoryFree(uint64_t *page_dir) {
  // uint32_t *prev_pagedir = GetPageDirectory();
  // ChangePageDirectory(page_dir);

  for (int pml4_index = 0; pml4_index < 512; pml4_index++) {
    if (!(page_dir[pml4_index] & PF_PRESENT) || page_dir[pml4_index] & PF_PS)
      continue;
    size_t *pdp = (size_t *)(PTE_GET_ADDR(page_dir[pml4_index]) + HHDMoffset);

    for (int pdp_index = 0; pdp_index < 512; pdp_index++) {
      if (!(pdp[pdp_index] & PF_PRESENT) || pdp[pdp_index] & PF_PS)
        continue;
      size_t *pd = (size_t *)(PTE_GET_ADDR(pdp[pdp_index]) + HHDMoffset);

      for (int pd_index = 0; pd_index < 512; pd_index++) {
        if (!(pd[pd_index] & PF_PRESENT) || pd[pd_index] & PF_PS)
          continue;
        size_t *pt = (size_t *)(PTE_GET_ADDR(pd[pd_index]) + HHDMoffset);

        for (int pt_index = 0; pt_index < 512; pt_index++) {
          if (!(pt[pt_index] & PF_PRESENT) || pt[pt_index] & PF_PS)
            continue;

          // we only free mappings related to userland (ones from ELF)
          if (!(pt[pt_index] & PF_USER))
            continue;

          uint64_t phys = PTE_GET_ADDR(pt[pt_index]);
          BitmapFreePageframe(&physical, (void *)phys);
        }
      }
    }
  }

  // ChangePageDirectory(prev_pagedir);
}
