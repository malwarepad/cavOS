#include "multiboot.h"
#include "types.h"
#include "util.h"

#define MAX_MEMORY_BLOCKS 32
#define BLOCK_SIZE 4096  // 4KB
#define BITS_PER_BLOCK 1 // 1 bit / block
#define MEMORY_BASE 0
// sizeof(BitmapUnitType) * 8 = sizeof(uint32_t) * 8 = 32
// BitmapUnitType* Bitmap;
#define BLOCKS_PER_UNIT 8

#define DivRoundUp(number, divisor) ((number + divisor - 1) / divisor)

#ifndef PMM_H
#define PMM_H

uint32_t *Bitmap;
uint32_t  BitmapSize;

void pmmTesting(multiboot_info_t *mbi);

#endif
