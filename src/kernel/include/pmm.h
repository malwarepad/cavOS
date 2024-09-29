#ifndef PMM_H
#define PMM_H

#include "bitmap.h"
#include "types.h"

DS_Bitmap physical;

void initiatePMM();

size_t PhysicalAllocate(int pages);
void   PhysicalFree(size_t ptr, int pages);

#endif
