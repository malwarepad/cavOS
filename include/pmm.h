#ifndef PMM_H
#define PMM_H

#include "multiboot2.h"
#include "types.h"

#define DivRoundUp(n, d) (((n) + (d)-1) / (d))

#define BLOCKS_PER_BYTE 32
#define BLOCK_SIZE 4096
#define INVALID_BLOCK ((uint32_t)-1)

void InitialiseBitmap();
void BitmapDump();
void BitmapDumpBlocks();

void  *BitmapAllocate(uint32_t blocks);
void   BitmapFree(void *base, uint32_t blocks);
uint32 BitmapAllocatePageframe();
void   BitmapFreePageframe(uint32_t addr);

void MarkRegion(void *basePtr, size_t sizeBytes, int isUsed);

#endif
