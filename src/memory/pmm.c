#include "../../include/pmm.h"

// Bitmap-based memory allocation algorythm
// Copyright (C) 2023 Panagiotis

void *ToPtr(uint32_t block) {
  uint8_t *u8Ptr = MEMORY_BASE + (block * BLOCK_SIZE);
  return (void *)(u8Ptr); // i hate my life
}

uint32_t ToBlock(void *ptr) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  return (uint32_t)(u8Ptr - MEMORY_BASE) / BLOCK_SIZE;
}

uint32_t ToBlockRoundUp(void *ptr) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  return (uint32_t)DivRoundUp((uint32_t)(u8Ptr - MEMORY_BASE), BLOCK_SIZE);
}

/*
before optimization:
  uint32_t currentRegionStart = 0;
  uint32_t currentRegionSize = 0;

  for (uint32_t i = 0; i < mbi_memorySize; i++) {
    if (Get(i)) {
      currentRegionSize = 0;
      currentRegionStart = i + 1;
    } else {
      currentRegionSize++;
      if (currentRegionSize >= blocks)
        return currentRegionStart;
    }
  }

  return INVALID_BLOCK;
*/

uint32_t FindFreeRegion(uint32_t blocks) {
  uint32_t currentRegionStart = 0;
  uint32_t currentRegionSize = 0;

  for (uint32_t i = 0; i < mbi_memorySize; i++) {
    if (Get(i)) {
      currentRegionSize = 0;
      currentRegionStart = i + 1;
    } else {
      currentRegionSize++;
      if (currentRegionSize >= blocks)
        return currentRegionStart;
    }
  }

  return INVALID_BLOCK;
}

void *BitmapAllocate(uint32_t blocks) {
  if (blocks == 0)
    return;

  uint32_t pickedRegion = FindFreeRegion(blocks);
  if (pickedRegion == INVALID_BLOCK) {
    return;
  }

  MarkBlocks(pickedRegion, blocks, 1);
  return ToPtr(pickedRegion);
}

void BitmapFree(void *base, uint32_t blocks) {
  MarkRegion(base, BLOCK_SIZE * blocks, 0);
}

void initiateBitmap() {
  // uint32_t memory_size =
  //     (mbi->mem_upper + 1024) * 1024; // mbi->mem_upper * 1024
  debugf("bish memory: %d\n", mbi_memorySize);
  BitmapSize = ((mbi_memorySize) / BLOCK_SIZE) * BITS_PER_BLOCK;
  BitmapHardLock = 0;
  // for (int i = 0; i < mbi->mmap_length; i += sizeof(multiboot_memory_map_t))
  // {
  //   multiboot_memory_map_t *mmmt =
  //       (multiboot_memory_map_t *)(mbi->mmap_addr + i);
  //   // if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
  //   debugf("[%x %x - %x %x] {%x}\n", mmmt->addr_low, mmmt->len_high,
  //          mmmt->len_low, mmmt->type);
  // }
  debugf("%d -> (%d / %d) * %d\n", BitmapSize, mbi_memorySize, BLOCK_SIZE,
         BITS_PER_BLOCK);

  uint8_t found = 0;

  multiboot_memory_map_t *mmmt;

  for (int i = 0; i < memoryMapCnt; i++) {
    mmmt = memoryMap[i];
    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE && (mmmt->len) >= BitmapSize) {
      // debugf("[%x %x - %x %x] {%x}\n", mmmt->addr_low, mmmt->len_high,
      //        mmmt->len_low, mmmt->type);
      found = 1;
      break;
    }
  }

  if (!found) {
    printf(
        "[+] Bitmap allocator: Not enough memory!\n> %d required, %d found!\n",
        BitmapSize, mbi_memorySize);
    asm("hlt");
    return 1;
  }

  debugf("Bitmap allocator's memory address: %x\n", mmmt->addr);

  Bitmap = (uint32_t *)(ToPtr(mmmt->addr));

  memset(Bitmap, 0xFF, BitmapSize);

  for (int i = 0; i < memoryMapCnt; i++) {
    mmmt = memoryMap[i];
    if (mmmt->addr > mbi_memorySize)
      continue;
    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
      MarkBlocks(ToBlockRoundUp(mmmt->addr), mmmt->len / BLOCK_SIZE, 0);
    }
  }

  for (int i = 0; i < memoryMapCnt; i++) {
    mmmt = memoryMap[i];
    if (mmmt->addr > mbi_memorySize)
      continue;
    if (mmmt->type != MULTIBOOT_MEMORY_AVAILABLE)
      MarkBlocks(ToBlock(mmmt->addr), DivRoundUp(mmmt->len, BLOCK_SIZE), 1);
  }

  MarkRegion(Bitmap, BitmapSize, 1);
}

void BitmapDebugDump() {
  for (int i = 0; i < 1024; i++) {
    int res = Get(i);
    if (i % 32 == 0)
      debugf("\n%06x ", i);
    debugf("%d ", res);
  }
  debugf("\n");
}

int Get(uint32_t block) {
  uint32_t addr = block / BLOCKS_PER_UNIT;
  uint32_t offset = block % BLOCKS_PER_UNIT;
  return (Bitmap[addr] & (1 << offset)) != 0;
}

void Set(uint32_t block, uint8_t value) {
  uint32_t addr = block / BLOCKS_PER_UNIT;
  uint32_t offset = block % BLOCKS_PER_UNIT;
  if (value)
    Bitmap[addr] |= (1 << offset);
  else
    Bitmap[addr] &= ~(1 << offset);
}

void MarkRegion(void *basePtr, size_t sizeBytes, int isUsed) {
  uint32_t base;
  size_t   size;

  if (isUsed) {
    base = ToBlock(basePtr);
    size = DivRoundUp(sizeBytes, BLOCK_SIZE);
  } else {
    base = ToBlockRoundUp(basePtr);
    size = sizeBytes / BLOCK_SIZE;
  }

  MarkBlocks(base, size, isUsed);
}

void MarkBlocks(uint32_t base, size_t size, uint8_t isUsed) {
  for (uint32_t i = base; i < base + size; i++)
    Set(i, isUsed);
}
