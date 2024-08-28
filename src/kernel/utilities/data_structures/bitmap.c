#include <bitmap.h>
#include <util.h>

// Bitmap-based memory region manager
// 10000000 -> first block is allocated, other are free :")
// Copyright (C) 2024 Panagiotis

/* Conversion utilities */

void *ToPtr(DS_Bitmap *bitmap, size_t block) {
  uint8_t *u8Ptr = (uint8_t *)(bitmap->mem_start + (block * BLOCK_SIZE));
  return (void *)(u8Ptr);
}

size_t ToBlock(DS_Bitmap *bitmap, void *ptr) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  return (size_t)(u8Ptr - bitmap->mem_start) / BLOCK_SIZE;
}

size_t ToBlockRoundUp(DS_Bitmap *bitmap, void *ptr) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  return (size_t)DivRoundUp((size_t)(u8Ptr - bitmap->mem_start), BLOCK_SIZE);
}

/* Bitmap data structure essentials */

size_t BitmapCalculateSize(size_t totalSize) {
  size_t BitmapSizeInBlocks = DivRoundUp(totalSize, BLOCK_SIZE);
  size_t BitmapSizeInBytes = DivRoundUp(BitmapSizeInBlocks, 8);
  return BitmapSizeInBytes;
}

int BitmapGet(DS_Bitmap *bitmap, size_t block) {
  size_t addr = block / BLOCKS_PER_BYTE;
  size_t offset = block % BLOCKS_PER_BYTE;
  return (bitmap->Bitmap[addr] & (1 << offset)) != 0;
}

void BitmapSet(DS_Bitmap *bitmap, size_t block, bool value) {
  size_t addr = block / BLOCKS_PER_BYTE;
  size_t offset = block % BLOCKS_PER_BYTE;
  if (value)
    bitmap->Bitmap[addr] |= (1 << offset);
  else
    bitmap->Bitmap[addr] &= ~(1 << offset);
}

/* Debugging functions */

#define BITMAP_DEBUG_F debugf
void BitmapDump(DS_Bitmap *bitmap) {
  BITMAP_DEBUG_F("=== BYTE DUMPING %d -> %d BYTES ===\n",
                 bitmap->BitmapSizeInBlocks, bitmap->BitmapSizeInBytes);
  for (int i = 0; i < bitmap->BitmapSizeInBytes; i++) {
    BITMAP_DEBUG_F("%x ", bitmap->Bitmap[i]);
  }
  BITMAP_DEBUG_F("\n");
}

void BitmapDumpBlocks(DS_Bitmap *bitmap) {
  BITMAP_DEBUG_F("=== BLOCK DUMPING %d (512-limited) ===\n",
                 bitmap->BitmapSizeInBlocks);
  for (int i = 0; i < 512; i++) {
    BITMAP_DEBUG_F("%d ", BitmapGet(bitmap, i));
  }
  BITMAP_DEBUG_F("\n");
}

/* Marking large chunks of memory */
void MarkBlocks(DS_Bitmap *bitmap, size_t start, size_t size, bool val) {
  // optimization(1): bitmap.h
  if (!val && start < bitmap->lastDeepFragmented)
    bitmap->lastDeepFragmented = start;

  for (size_t i = start; i < start + size; i++) {
    BitmapSet(bitmap, i, val);
  }
}

void MarkRegion(DS_Bitmap *bitmap, void *basePtr, size_t sizeBytes,
                int isUsed) {
  size_t base;
  size_t size;

  if (isUsed) {
    base = ToBlock(bitmap, basePtr);
    size = DivRoundUp(sizeBytes, BLOCK_SIZE);
  } else {
    base = ToBlockRoundUp(bitmap, basePtr);
    size = sizeBytes / BLOCK_SIZE;
  }

  // debugf("MARKING REGION! %x len{%x} %d\n", base, size, isUsed);
  MarkBlocks(bitmap, base, size, isUsed);
}

size_t FindFreeRegion(DS_Bitmap *bitmap, size_t blocks) {
  size_t currentRegionStart = bitmap->lastDeepFragmented;
  size_t currentRegionSize = 0;

  for (size_t i = currentRegionStart; i < bitmap->BitmapSizeInBlocks; i++) {
    if (BitmapGet(bitmap, i)) {
      currentRegionSize = 0;
      currentRegionStart = i + 1;
    } else {
      // optimization(1): bitmap.h
      if (blocks == 1)
        bitmap->lastDeepFragmented = currentRegionStart + 1;

      currentRegionSize++;
      if (currentRegionSize >= blocks)
        return currentRegionStart;
    }
  }

  debugf("[bitmap] Didn't find jack shit!\n");
  return INVALID_BLOCK;
}

void *BitmapAllocate(DS_Bitmap *bitmap, size_t blocks) {
  if (blocks == 0)
    return 0;

  size_t pickedRegion = FindFreeRegion(bitmap, blocks);
  if (pickedRegion == INVALID_BLOCK)
    return 0;

  MarkBlocks(bitmap, pickedRegion, blocks, 1);
  return ToPtr(bitmap, pickedRegion);
}

void BitmapFree(DS_Bitmap *bitmap, void *base, size_t blocks) {
  MarkRegion(bitmap, base, BLOCK_SIZE * blocks, 0);
}

/* Pageframes (1 block) */

size_t BitmapAllocatePageframe(DS_Bitmap *bitmap) {
  size_t pickedRegion = FindFreeRegion(bitmap, 1);
  // if (pickedRegion == INVALID_BLOCK) {
  //   printf("no!");
  //   panic();
  // }
  MarkBlocks(bitmap, pickedRegion, 1, 1);

  // debugf("[%x] memallocpageframe: %x\n", &bitmap->Bitmap,
  //        (bitmap->mem_start + (pickedRegion * BLOCK_SIZE)));

  return (bitmap->mem_start + (pickedRegion * BLOCK_SIZE));
}

void BitmapFreePageframe(DS_Bitmap *bitmap, void *addr) {
  MarkRegion(bitmap, addr, BLOCK_SIZE * 1, 0);
}