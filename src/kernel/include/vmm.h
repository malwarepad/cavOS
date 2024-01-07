#include "bitmap.h"
#include "types.h"

#ifndef VMM_H
#define VMM_H

uint32_t sysalloc_base;
#define KERNEL_SPACE 1073741824 // 1 gig after loc 0xC0000000

DS_Bitmap virtual;

typedef struct PhysicallyContiguous {
  uint32_t phys;
  uint32_t virt;
} PhysicallyContiguous;

void                *VirtualAllocate(int pages);
PhysicallyContiguous VirtualAllocatePhysicallyContiguous(int pages);
int                  VirtualFree(void *ptr, int pages);

#endif