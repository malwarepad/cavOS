#include "types.h"

#ifndef BITMAP_H
#define BITMAP_H

typedef struct DS_Bitmap {
  uint8_t *Bitmap;
  uint32_t BitmapSizeInBlocks; // CEIL(x / BLOCK_SIZE)
  uint32_t BitmapSizeInBytes;  // CEIL(blockSize / 8)

  uint32_t mem_start;
  bool     ready; // has been initiated
} DS_Bitmap;

#define DivRoundUp(n, d) (((n) + (d)-1) / (d))

#define BLOCKS_PER_BYTE 8 // using uint8_t
#define BLOCK_SIZE 4096
#define INVALID_BLOCK ((uint32_t)-1)

void    *ToPtr(DS_Bitmap *bitmap, uint32_t block);
uint32_t ToBlock(DS_Bitmap *bitmap, void *ptr);
uint32_t ToBlockRoundUp(DS_Bitmap *bitmap, void *ptr);

int  BitmapGet(DS_Bitmap *bitmap, uint32_t block);
void BitmapSet(DS_Bitmap *bitmap, uint32_t block, bool value);

void BitmapDump(DS_Bitmap *bitmap);
void BitmapDumpBlocks(DS_Bitmap *bitmap);

void     MarkBlocks(DS_Bitmap *bitmap, uint32_t start, uint32_t size, bool val);
void     MarkRegion(DS_Bitmap *bitmap, void *basePtr, uint32_t sizeBytes,
                    int isUsed);
uint32_t FindFreeRegion(DS_Bitmap *bitmap, uint32_t blocks);
void    *BitmapAllocate(DS_Bitmap *bitmap, uint32_t blocks);

uint32_t BitmapAllocatePageframe(DS_Bitmap *bitmap);

#endif