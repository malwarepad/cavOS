#include "bitmap.h"
#include "types.h"

#ifndef VMM_H
#define VMM_H

DS_Bitmap virtual;

void initiateVMM();

void *VirtualAllocate(int pages);
void *VirtualAllocatePhysicallyContiguous(int pages);
bool  VirtualFree(void *ptr, int pages);

#endif