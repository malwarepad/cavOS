#include <bitmap.h>

// Bitmap-based memory region manager
// 10000000 -> first block is allocated, other are free :")
// Copyright (C) 2023 Panagiotis

/* Conversion utilities */

void *ToPtr(DS_Bitmap *bitmap, uint32_t block) {
  uint8_t *u8Ptr = (uint8_t *)(bitmap->mem_start + (block * BLOCK_SIZE));
  return (void *)(u8Ptr);
}

uint32_t ToBlock(DS_Bitmap *bitmap, void *ptr) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  return (uint32_t)(u8Ptr - bitmap->mem_start) / BLOCK_SIZE;
}

uint32_t ToBlockRoundUp(DS_Bitmap *bitmap, void *ptr) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  return (uint32_t)DivRoundUp((uint32_t)(u8Ptr - bitmap->mem_start),
                              BLOCK_SIZE);
}

/* Bitmap data structure essentials */

int BitmapGet(DS_Bitmap *bitmap, uint32_t block) {
  uint32_t addr = block / BLOCKS_PER_BYTE;
  uint32_t offset = block % BLOCKS_PER_BYTE;
  return (bitmap->Bitmap[addr] & (1 << offset)) != 0;
}

void BitmapSet(DS_Bitmap *bitmap, uint32_t block, bool value) {
  uint32_t addr = block / BLOCKS_PER_BYTE;
  uint32_t offset = block % BLOCKS_PER_BYTE;
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
void MarkBlocks(DS_Bitmap *bitmap, uint32_t start, uint32_t size, bool val) {
  for (uint32_t i = start; i < start + size; i++) {
    BitmapSet(bitmap, i, val);
  }
}

void MarkRegion(DS_Bitmap *bitmap, void *basePtr, uint32_t sizeBytes,
                int isUsed) {
  uint32_t base;
  uint32_t size;

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

uint32_t FindFreeRegion(DS_Bitmap *bitmap, uint32_t blocks) {
  uint32_t currentRegionStart = 0;
  uint32_t currentRegionSize = 0;

  for (uint32_t i = 0; i < bitmap->BitmapSizeInBlocks; i++) {
    if (BitmapGet(bitmap, i)) {
      currentRegionSize = 0;
      currentRegionStart = i + 1;
    } else {
      currentRegionSize++;
      if (currentRegionSize >= blocks)
        return currentRegionStart;
    }
  }

  debugf("[bitmap] Didn't find jack shit!\n");
  return INVALID_BLOCK;
}

void *BitmapAllocate(DS_Bitmap *bitmap, uint32_t blocks) {
  if (blocks == 0)
    return;

  uint32_t pickedRegion = FindFreeRegion(bitmap, blocks);
  // if (pickedRegion == INVALID_BLOCK) {
  //   printf("no!");
  //   panic();
  // }

  MarkBlocks(bitmap, pickedRegion, blocks, 1);
  return ToPtr(bitmap, pickedRegion);
}

void BitmapFree(DS_Bitmap *bitmap, void *base, uint32_t blocks) {
  MarkRegion(bitmap, base, BLOCK_SIZE * blocks, 0);
}

/* Pageframes (1 block) */

uint32_t BitmapAllocatePageframe(DS_Bitmap *bitmap) {
  uint32_t pickedRegion = FindFreeRegion(bitmap, 1);
  // if (pickedRegion == INVALID_BLOCK) {
  //   printf("no!");
  //   panic();
  // }
  MarkBlocks(bitmap, pickedRegion, 1, 1);

  // debugf("[%x] memallocpageframe: %x\n", &bitmap->Bitmap,
  //        (bitmap->mem_start + (pickedRegion * BLOCK_SIZE)));

  return (bitmap->mem_start + (pickedRegion * BLOCK_SIZE));
}

void BitmapFreePageframe(DS_Bitmap *bitmap, uint32_t addr) {
  MarkRegion(bitmap, addr, BLOCK_SIZE * 1, 0);
}