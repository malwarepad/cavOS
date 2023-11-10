#include "types.h"

#ifndef VMM_H
#define VMM_H

uint32_t sysalloc_base;

void *VirtualAllocate(int pages);
int   VirtualFree(void *ptr, int pages);

#endif