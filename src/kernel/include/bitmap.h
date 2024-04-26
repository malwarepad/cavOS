#include "types.h"

#ifndef BITMAP_H
#define BITMAP_H

typedef struct DS_Bitmap {
  uint8_t *Bitmap;
  size_t   BitmapSizeInBlocks; // CEIL(x / BLOCK_SIZE)
  size_t   BitmapSizeInBytes;  // CEIL(blockSize / 8)

  size_t mem_start;
  bool   ready; // has been initiated
} DS_Bitmap;

#define BLOCKS_PER_BYTE 8 // using uint8_t
#define BLOCK_SIZE 4096
#define INVALID_BLOCK ((size_t)-1)

void  *ToPtr(DS_Bitmap *bitmap, size_t block);
size_t ToBlock(DS_Bitmap *bitmap, void *ptr);
size_t ToBlockRoundUp(DS_Bitmap *bitmap, void *ptr);

size_t BitmapCalculateSize(size_t totalSize);
int    BitmapGet(DS_Bitmap *bitmap, size_t block);
void   BitmapSet(DS_Bitmap *bitmap, size_t block, bool value);

void BitmapDump(DS_Bitmap *bitmap);
void BitmapDumpBlocks(DS_Bitmap *bitmap);

void MarkBlocks(DS_Bitmap *bitmap, size_t start, size_t size, bool val);
void MarkRegion(DS_Bitmap *bitmap, void *basePtr, size_t sizeBytes, int isUsed);
size_t FindFreeRegion(DS_Bitmap *bitmap, size_t blocks);
void  *BitmapAllocate(DS_Bitmap *bitmap, size_t blocks);

size_t BitmapAllocatePageframe(DS_Bitmap *bitmap);
void   BitmapFreePageframe(DS_Bitmap *bitmap, void *addr);

#endif