#include <liballoc.h>
#include <paging.h>
#include <types.h>
#include <util.h>

// System-wide page table & directory management
// Copyright (C) 2023 Panagiotis

static void enable_paging();
static void invalidate(int vaddr);

int mem_num_vpages;

#define NUM_PAGE_DIRS 256
static uint32_t page_dirs[NUM_PAGE_DIRS][1024] __attribute__((aligned(4096)));
static uint8_t  page_dir_used[NUM_PAGE_DIRS];

// only needed for max 32 4096-blocks
uint32_t tmppageframeStart = 0;
void     registerTmpPageFrame(uint32_t target) { tmppageframeStart = target; }
uint32_t TempPageFrame() {
  uint32_t fin = tmppageframeStart;
  tmppageframeStart += PAGE_SIZE;
  return fin;
}

void initiatePaging() {
  // unmap the first 4 mb
  initial_page_dir[0] = 0;
  invalidate(0);

  // recursive table mapping
  initial_page_dir[1023] = ((uint32_t)initial_page_dir - 0xC0000000) |
                           PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE;
  invalidate(0xFFFFF000);

  memset(page_dirs, 0, 0x1000 * NUM_PAGE_DIRS);
  memset(page_dir_used, 0, NUM_PAGE_DIRS);
}

void ChangePageDirectory(uint32_t *pd) {
  if (pd > 0xC0000000)
    ;
  pd = (uint32_t *)(((uint32_t)pd) - 0xC0000000); // calc the physical address
  asm volatile("mov %0, %%eax \n"
               "mov %%eax, %%cr3 \n" ::"m"(pd));
}

uint32_t *GetPageDirectory() {
  uint32_t pd;
  asm volatile("mov %%cr3, %0" : "=r"(pd));
  pd += 0xC0000000; // calc the virtual address
  return (uint32_t *)pd;
}

void enable_paging() {
  asm volatile("mov %cr4, %ebx \n"
               "or $0x10, %ebx \n"
               "mov %ebx, %cr4 \n"

               "mov %cr0, %ebx \n"
               "or $0x80000000, %ebx \n"
               "mov %ebx, %cr0 \n");
}

void invalidate(int vaddr) { asm volatile("invlpg %0" ::"m"(vaddr)); }

// addresses need to be 4096 aligned
void VirtualMap(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags) {
  uint32_t *prev_page_dir = 0;
  if (virt_addr >= 0xC0000000) {
    // optimization: just copy from current pagedir to all others, including
    // init_page_dir
    //              we might just wanna have init_page_dir be page_dirs[0]
    //              instead then we would have to build it in boot.asm in
    //              assembly

    // write to initial_page_dir, so that we can sync that across all others

    prev_page_dir = GetPageDirectory();

    if (prev_page_dir != initial_page_dir)
      ChangePageDirectory(initial_page_dir);
  }

  // extract indices from the vaddr
  uint32_t pd_index = virt_addr >> 22;
  uint32_t pt_index = virt_addr >> 12 & 0x3FF;

  uint32_t *page_dir = REC_PAGEDIR;

  // page tables can only be directly accessed/modified using the recursive
  // strat? > yes since their physical page is not mapped into memory
  uint32_t *pt = REC_PAGETABLE(pd_index);

  if (!(page_dir[pd_index] & PAGE_FLAG_PRESENT)) {
    // allocate a page table
    uint32_t pt_paddr =
        physical.ready ? BitmapAllocatePageframe(&physical) : TempPageFrame();

    page_dir[pd_index] = pt_paddr | PAGE_FLAG_PRESENT | PAGE_FLAG_WRITE |
                         PAGE_FLAG_OWNER | flags;
    invalidate(virt_addr);

    // we can now access it directly using the recursive strategy
    for (uint32_t i = 0; i < 1024; i++) {
      pt[i] = 0;
    }
  }

  pt[pt_index] = phys_addr | PAGE_FLAG_PRESENT | flags;
  mem_num_vpages++;
  invalidate(virt_addr);

  if (prev_page_dir != 0) {
    // ... then sync that across all others
    SyncPageDirectory();
    // we changed to init page dir, now we need to change back
    if (prev_page_dir != initial_page_dir)
      ChangePageDirectory(prev_page_dir);
  }
}

void *VirtualToPhysical(uint32_t virt_addr) {
  uint32_t pd_index = virt_addr >> 22;
  uint32_t pt_index = virt_addr >> 12 & 0x3FF;

  uint32_t *pd = REC_PAGEDIR;
  uint32_t *pt = REC_PAGETABLE(pd_index);

  return (void *)((pt[pt_index] & ~0xFFF) + ((uint32_t)virt_addr & 0xFFF));
}

// returns page table entry (physical address and flags)
uint32_t VirtualUnmap(uint32_t virt_addr) {
  uint32_t *prev_page_dir = 0;
  if (virt_addr >= 0xC0000000) {
    // optimization: just copy from current pagedir to all others, including
    // init_page_dir
    //              we might just wanna have init_page_dir be page_dirs[0]
    //              instead then we would have to build it in boot.asm in
    //              assembly

    // write to initial_page_dir, so that we can sync that across all others

    prev_page_dir = GetPageDirectory();

    if (prev_page_dir != initial_page_dir)
      ChangePageDirectory(initial_page_dir);
  }

  uint32_t pd_index = virt_addr >> 22;
  uint32_t pt_index = virt_addr >> 12 & 0x3FF;

  uint32_t *page_dir = REC_PAGEDIR;
  uint32_t *pt = REC_PAGETABLE(pd_index);

  uint32_t pte = pt[pt_index];
  pt[pt_index] = 0;
  mem_num_vpages--;

  bool remove = true;
  // optimization: keep track of the number of pages present in each page table
  for (uint32_t i = 0; i < 1024; i++) {
    if (pt[i] & PAGE_FLAG_PRESENT) {
      remove = false;
      break;
    }
  }

  if (remove) {
    // table is empty, destroy its physical frame if we own it.
    uint32_t pde = page_dir[pd_index];
    if (pde & PAGE_FLAG_OWNER) {
      uint32_t pt_paddr = P_PHYS_ADDR(pde);
      BitmapFreePageframe(pt_paddr);
      page_dir[pd_index] = 0;
    }
  }

  invalidate(virt_addr);

  // free it here for now
  if (pte & PAGE_FLAG_OWNER) {
    BitmapFreePageframe(P_PHYS_ADDR(pte));
  }

  if (prev_page_dir != 0) {
    // ... then sync that across all others
    SyncPageDirectory();
    // we changed to init page dir, now we need to change back
    if (prev_page_dir != initial_page_dir)
      ChangePageDirectory(prev_page_dir);
  }

  return pte;
}

uint32_t *PageDirectoryAllocate() {
  for (int i = 0; i < NUM_PAGE_DIRS; i++) {
    if (!page_dir_used[i]) {
      page_dir_used[i] = true;

      uint32_t *page_dir = page_dirs[i];
      memset(page_dir, 0, 0x1000);

      // first 768 entries are user page tables
      for (int i = 0; i < 768; i++) {
        page_dir[i] = 0;
      }

      // next 256 are kernel (except last)
      for (int i = 768; i < 1023; i++) {
        page_dir[i] = initial_page_dir[i] & ~PAGE_FLAG_OWNER;
      }

      // recursive mapping
      page_dir[1023] = (((uint32_t)page_dir) - 0xC0000000) | PAGE_FLAG_PRESENT |
                       PAGE_FLAG_WRITE;
      return page_dir;
    }
  }

  return 0;
}

void PageDirectoryFree(uint32_t *page_dir) {
  uint32_t *prev_pagedir = GetPageDirectory();
  ChangePageDirectory(page_dir);

  uint32_t pagedir_index = ((uint32_t)page_dir) - ((uint32_t)page_dirs);
  pagedir_index /= 0x1000;

  uint32_t *pd = REC_PAGEDIR;
  for (int i = 0; i < 768; i++) {
    int pde = pd[i];
    if (pde == 0)
      continue;

    uint32_t *ptable = REC_PAGETABLE(i);
    for (int j = 0; j < 1024; j++) {
      uint32_t pte = ptable[j];

      if (pte & PAGE_FLAG_OWNER) {
        BitmapFreePageframe(P_PHYS_ADDR(pte));
      }
    }
    memset(ptable, 0, 0x1000);

    if (pde & PAGE_FLAG_OWNER) {
      BitmapFreePageframe(P_PHYS_ADDR(pde));
    }
    pd[i] = 0;
  }

  page_dir_used[pagedir_index] = 0;
  ChangePageDirectory(prev_pagedir);
}

void SyncPageDirectory() {
  for (int i = 0; i < NUM_PAGE_DIRS; i++) {
    if (page_dir_used[i]) {
      uint32_t *page_dir = page_dirs[i];

      for (int i = 768; i < 1023; i++) {
        page_dir[i] = initial_page_dir[i] & ~PAGE_FLAG_OWNER;
      }
    }
  }
}
