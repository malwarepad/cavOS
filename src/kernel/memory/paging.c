#include <bitmap.h>
#include <bootloader.h>
#include <fb.h>
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
  uint64_t pdPhys = 0;
  asm volatile("movq %%cr3,%0" : "=r"(pdPhys));
  if (!pdPhys) {
    debugf("[paging] Could not parse default pagedir!\n");
    panic();
  }

  uint64_t pdVirt = pdPhys + bootloader.hhdmOffset;
  globalPagedir = (uint64_t *)pdVirt;

  // VirtualSeek(bootloader.hhdmOffset);
}

void VirtualMapRegionByLength(uint64_t virt_addr, uint64_t phys_addr,
                              uint64_t length, uint64_t flags) {
#if ELF_DEBUG
  debugf("[paging::map::region] virt{%lx} phys{%lx} len{%lx}\n", virt_addr,
         phys_addr, length);
#endif
  if (length == 0) return;
  uint32_t pagesAmnt = DivRoundUp(length, PAGE_SIZE);
  uint64_t xvirt = virt_addr;
  uint64_t xphys = phys_addr;
  for (uint32_t i = 0; i < pagesAmnt; i++, xvirt += PAGE_SIZE, xphys += PAGE_SIZE) {
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

// Used by the scheduler to avoid accessing globalPagedir directly
void ChangePageDirectoryFake(uint64_t *pd) {
  if (!VirtualToPhysical((size_t)pd)) {
    debugf("[paging] Could not (fake) change to pd{%lx}!\n", pd);
    panic();
  }
  globalPagedir = pd;
}

void ChangePageDirectory(uint64_t *pd) {
  if (tasksInitiated) {
    spinlockAcquire(&currentTask->infoPd->LOCK_PD);
    if (pd == currentTask->infoPd->pagedir)
      currentTask->pagedirOverride = 0;
    else
      currentTask->pagedirOverride = pd;
    spinlockRelease(&currentTask->infoPd->LOCK_PD);
  }
  ChangePageDirectoryUnsafe(pd);
}

inline uint64_t *GetPageDirectory() { return (uint64_t *)globalPagedir; }

inline uint64_t *GetTaskPageDirectory(const void *taskPtr) {
  const Task *task = (const Task *)taskPtr;
  TaskInfoPagedir *info = task->infoPd;
  spinlockAcquire(&info->LOCK_PD);
  uint64_t *ret = task->pagedirOverride ? task->pagedirOverride : info->pagedir;
  spinlockRelease(&info->LOCK_PD);
  return ret;
}

inline void invalidate(uint64_t vaddr) { asm volatile("invlpg %0" ::"m"(vaddr)); }

size_t PagingPhysAllocate() {
  size_t phys = PhysicalAllocate(1);
  void *virt = (void *)(phys + HHDMoffset);
  memset(virt, 0, PAGE_SIZE);
  return phys;
}

SpinlockCnt WLOCK_PAGING = {0};

void VirtualMap(uint64_t virt_addr, uint64_t phys_addr, uint64_t flags) {
  VirtualMapL(globalPagedir, virt_addr, phys_addr, flags);
}

void VirtualMapL(uint64_t *restrict pagedir, uint64_t virt_addr, uint64_t phys_addr,
                 uint64_t flags) {
  if (virt_addr % PAGE_SIZE) {
    debugf("[paging] Tried to map non-aligned address! virt{%lx} phys{%lx}\n",
           virt_addr, phys_addr);
    panic();
  }
  virt_addr = AMD64_MM_STRIPSX(virt_addr);

  const uint32_t pml4_index = PML4E(virt_addr);
  const uint32_t pdp_index = PDPTE(virt_addr);
  const uint32_t pd_index = PDE(virt_addr);
  const uint32_t pt_index = PTE(virt_addr);

  spinlockCntWriteAcquire(&WLOCK_PAGING);
  size_t *pdp, *pd, *pt;
  if (!(pagedir[pml4_index] & PF_PRESENT)) {
    size_t target = PagingPhysAllocate();
    pagedir[pml4_index] = target | PF_PRESENT | PF_RW | PF_USER;
  }
  pdp = (size_t *)(PTE_GET_ADDR(pagedir[pml4_index]) + HHDMoffset);

  if (!(pdp[pdp_index] & PF_PRESENT)) {
    size_t target = PagingPhysAllocate();
    pdp[pdp_index] = target | PF_PRESENT | PF_RW | PF_USER;
  }
  pd = (size_t *)(PTE_GET_ADDR(pdp[pdp_index]) + HHDMoffset);

  if (!(pd[pd_index] & PF_PRESENT)) {
    size_t target = PagingPhysAllocate();
    pd[pd_index] = target | PF_PRESENT | PF_RW | PF_USER;
  }
  pt = (size_t *)(PTE_GET_ADDR(pd[pd_index]) + HHDMoffset);

  if (pt[pt_index] & PF_PRESENT &&
      !(PTE_GET_ADDR(pt[pt_index]) >= fb.phys &&
        PTE_GET_ADDR(pt[pt_index]) < fb.phys + (fb.width * fb.height * 4))) {
    PhysicalFree(PTE_GET_ADDR(pt[pt_index]), 1);
  }
  if (!phys_addr) // todo: proper unmapping
    pt[pt_index] = 0;
  else
    pt[pt_index] = (P_PHYS_ADDR(phys_addr)) | PF_PRESENT | flags;

  invalidate(virt_addr);
  spinlockCntWriteRelease(&WLOCK_PAGING);
#if ELF_DEBUG
  debugf("[paging] Mapped virt{%lx} to phys{%lx}\n", virt_addr, phys_addr);
#endif
}

// todo: maybe use atomic operations here since it's called from volatile
// contexts (id est. scheduler or signal returns)
size_t VirtualToPhysicalL(uint64_t *pagedir, size_t virt_addr) {
  if (!pagedir)
    return 0;

  // do note that we are on hhdm revision 0!
  if (virt_addr >= HHDMoffset &&
      virt_addr <= (HHDMoffset + MAX(bootloader.mmTotal, UINT32_MAX)))
    return virt_addr - HHDMoffset;

  size_t virt_addr_init = virt_addr;
  virt_addr &= ~0xFFF;

  virt_addr = AMD64_MM_STRIPSX(virt_addr);

  uint32_t pml4_index = PML4E(virt_addr);
  uint32_t pdp_index = PDPTE(virt_addr);
  uint32_t pd_index = PDE(virt_addr);
  uint32_t pt_index = PTE(virt_addr);

  // spinlockCntReadAcquire(&WLOCK_PAGING);
  if (!(pagedir[pml4_index] & PF_PRESENT))
    goto error;
  /*else if (pagedir[pml4_index] & PF_PRESENT &&
           pagedir[pml4_index] & PF_PS)
    return (void *)(PTE_GET_ADDR(pagedir[pml4_index] +
                                 (virt_addr & PAGE_MASK(12 + 9 + 9 + 9))));*/
  size_t *pdp = (size_t *)(PTE_GET_ADDR(pagedir[pml4_index]) + HHDMoffset);

  if (!(pdp[pdp_index] & PF_PRESENT))
    goto error;
  /*else if (pdp[pdp_index] & PF_PRESENT && pdp[pdp_index] & PF_PS)
    return (void *)(PTE_GET_ADDR(pdp[pdp_index] +
                                 (virt_addr & PAGE_MASK(12 + 9 + 9))));*/
  size_t *pd = (size_t *)(PTE_GET_ADDR(pdp[pdp_index]) + HHDMoffset);

  if (!(pd[pd_index] & PF_PRESENT))
    goto error;
  /*else if (pd[pd_index] & PF_PRESENT && pd[pd_index] & PF_PS)
    return (
        void *)(PTE_GET_ADDR(pd[pd_index] + (virt_addr & PAGE_MASK(12 + 9))));*/
  size_t *pt = (size_t *)(PTE_GET_ADDR(pd[pd_index]) + HHDMoffset);

  if (pt[pt_index] & PF_PRESENT) {
    // spinlockCntReadRelease(&WLOCK_PAGING);
    return (size_t)(PTE_GET_ADDR(pt[pt_index]) +
                    ((size_t)virt_addr_init & 0xFFF));
  }

error:
  // spinlockCntReadRelease(&WLOCK_PAGING);
  return 0;
}

size_t VirtualToPhysical(size_t virt_addr) {
  return VirtualToPhysicalL(globalPagedir, virt_addr);
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

  uint64_t *model = GetTaskPageDirectory(taskGet(KERNEL_TASK_ID));
  for (int i = 0; i < 512; i++)
    out[i] = model[i];

  return out;
}

// todo: clear orphans after a whole page level is emptied!
// destroys any userland stuff on the page directory
void PageDirectoryFree(uint64_t *page_dir) {
  spinlockCntWriteAcquire(&WLOCK_PAGING);

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
          PhysicalFree(phys, 1);
          pt[pt_index] = 0;
        }
      }
    }
  }

  spinlockCntWriteRelease(&WLOCK_PAGING);
}

void PageDirectoryUserDuplicate(uint64_t *source, uint64_t *target) {
  spinlockCntReadAcquire(&WLOCK_PAGING);
  for (int pml4_index = 0; pml4_index < 512; pml4_index++) {
    if (!(source[pml4_index] & PF_PRESENT) || source[pml4_index] & PF_PS)
      continue;
    size_t *pdp = (size_t *)(PTE_GET_ADDR(source[pml4_index]) + HHDMoffset);

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

          // we only duplicate mappings related to userland (ones from ELF)
          if (!(pt[pt_index] & PF_USER))
            continue;

          size_t physSource = PTE_GET_ADDR(pt[pt_index]);
          size_t physTarget =
              (pt[pt_index] & PF_SHARED) ? physSource : PagingPhysAllocate();

          size_t virt =
              BITS_TO_VIRT_ADDR(pml4_index, pdp_index, pd_index, pt_index);

          void *ptrSource = (void *)(physSource + HHDMoffset);
          void *ptrTarget = (void *)(physTarget + HHDMoffset);

          memcpy(ptrTarget, ptrSource, PAGE_SIZE);

          spinlockCntReadRelease(&WLOCK_PAGING);
          VirtualMapL(target, virt, physTarget, PF_RW | PF_USER);
          spinlockCntReadAcquire(&WLOCK_PAGING);
        }
      }
    }
  }

  spinlockCntReadRelease(&WLOCK_PAGING);
}
