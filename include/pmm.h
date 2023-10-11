#include "multiboot2.h"
#include "types.h"
#include "util.h"

#define MAX_MEMORY_BLOCKS 32
#define BLOCK_SIZE 4096  // 4KB
#define BITS_PER_BLOCK 1 // 1 bit / block
#define MEMORY_BASE 0
// sizeof(BitmapUnitType) * 8 = sizeof(uint32_t) * 8 = 32
// BitmapUnitType* Bitmap;
#define BLOCKS_PER_UNIT 8

#define INVALID_BLOCK ((uint64_t)-1)

#ifndef PMM_H
#define PMM_H

uint32_t *Bitmap;
uint32_t  BitmapSize;

uint8_t BitmapHardLock;

void  initiateBitmap();
void *BitmapAllocate(uint32_t blocks);
void  BitmapFree(void *base, uint32_t blocks);
void  BitmapDebugDump();

#endif
