#include "types.h"

#ifndef PAGING_H
#define PAGING_H

#define USER_STACK_PAGES (0x10)
#define PAGE_SIZE 0x1000

#define USER_HEAP_START 0x20000000
#define USER_STACK_BOTTOM 0xB0000000
#define USER_SHARED_MEMORY 0xB0000000
#define KERNEL_START 0xC0000000
#define KERNEL_GFX 0xC8000000
#define KERNEL_MALLOC 0xD0000000
#define KERNEL_SHARED_MEMORY 0xF0000000

#define PAGE_FLAG_PRESENT (1 << 0)
#define PAGE_FLAG_WRITE (1 << 1)
#define PAGE_FLAG_USER (1 << 2)
#define PAGE_FLAG_4MB (1 << 7)
#define PAGE_FLAG_OWNER (1 << 9) // we are in charge of the physical page

#define P_PHYS_ADDR(x) ((x) & ~0xFFF)

// these are virtual addresses
#define REC_PAGEDIR ((uint32_t *)0xFFFFF000)
#define REC_PAGETABLE(i) ((uint32_t *)(0xFFC00000 + ((i) << 12)))

extern uint32_t initial_page_dir[1024];
extern int      mem_num_vpages;

void initiatePaging();

void     VirtualMap(uint32_t virt_addr, uint32_t phys_addr, uint32_t flags);
uint32_t VirtualUnmap(uint32_t virt_addr);

void *VirtualToPhysical(uint32_t virt_addr);

uint32_t *GetPageDirectory();
void      ChangePageDirectory(uint32_t *pd);
void      SyncPageDirectory();

uint32_t *PageDirectoryAllocate();
void      PageDirectoryFree(uint32_t *page_dir);

#endif