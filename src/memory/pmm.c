#include "../../include/pmm.h"

// Bitmap-based memory allocation algorythm
// Copyright (C) 2023 Panagiotis

uint32_t *Bitmap;
uint32_t  BitmapSizeInBlocks = 0;
uint32_t  BitmapSizeInBytes = 0;

uint64_t mem_start = 0;

/* Conversion utilities */

void *ToPtr(uint32_t block) {
  uint8_t *u8Ptr = (uint8_t *)(mem_start + (block * BLOCK_SIZE));
  return (void *)(u8Ptr);
}

uint32_t ToBlock(void *ptr) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  return (uint32_t)(u8Ptr - mem_start) / BLOCK_SIZE;
}

uint32_t ToBlockRoundUp(void *ptr) {
  uint8_t *u8Ptr = (uint8_t *)ptr;
  return (uint32_t)DivRoundUp((uint32_t)(u8Ptr - mem_start), BLOCK_SIZE);
}

/* Bitmap data structure essentials */

int Get(uint32_t block) {
  uint32_t addr = block / BLOCKS_PER_BYTE;
  uint32_t offset = block % BLOCKS_PER_BYTE;
  return (Bitmap[addr] & (1 << offset)) != 0;
}

void Set(uint32_t block, bool value) {
  uint32_t addr = block / BLOCKS_PER_BYTE;
  uint32_t offset = block % BLOCKS_PER_BYTE;
  if (value)
    Bitmap[addr] |= (1 << offset);
  else
    Bitmap[addr] &= ~(1 << offset);
}

/* Debugging functions */

void BitmapDump() {
  uint32_t repeatInterval = DivRoundUp(BitmapSizeInBlocks, BLOCKS_PER_BYTE);
  printf("=== BYTE DUMPING %d -> %d BLOCKS ===\n", BitmapSizeInBlocks,
         repeatInterval);
  for (int i = 0; i < repeatInterval; i++) {
    printf("%x ", Bitmap[i]);
  }
  printf("\n");
}

void BitmapDumpBlocks() {
  printf("=== BLOCK DUMPING %d ===\n", BitmapSizeInBlocks);
  for (int i = 0; i < 512; i++) {
    printf("%d ", Get(i));
  }
  printf("\n");
}

/* Marking large chunks of memory */

void MarkBlocks(uint32_t start, uint32_t size, bool val) {
  for (uint32_t i = start; i < start + size; i++) {
    Set(i, val);
  }
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

uint32_t FindFreeRegion(uint32_t blocks) {
  uint32_t currentRegionStart = 0;
  uint32_t currentRegionSize = 0;

  for (uint32_t i = 0; i < BitmapSizeInBlocks; i++) {
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
    printf("no!");
    asm("cli");
    asm("hlt");
  }

  MarkBlocks(pickedRegion, blocks, 1);
  return ToPtr(pickedRegion);
}

void BitmapFree(void *base, uint32_t blocks) {
  MarkRegion(base, BLOCK_SIZE * blocks, 0);
}

void initiateBitmap() {
  BitmapSizeInBlocks = DivRoundUp(mbi_memorySize, BLOCK_SIZE);
  BitmapSizeInBytes = DivRoundUp(BitmapSizeInBlocks, BLOCKS_PER_BYTE);

  uint8_t found = 0;

  multiboot_memory_map_t *mmmt;

  for (int i = 0; i < memoryMapCnt; i++) {
    mmmt = memoryMap[i];
    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE &&
        (mmmt->len) >= BitmapSizeInBytes) {
      // debugf("[%x %x - %x %x] {%x}\n", mmmt->addr_low, mmmt->len_high,
      //        mmmt->len_low, mmmt->type);
      found = 1;
      break;
    }
  }

  if (!found) {
    printf(
        "[+] Bitmap allocator: Not enough memory!\n> %d required, %d found!\n",
        BitmapSizeInBytes, mbi_memorySize);
    asm("cli");
    asm("hlt");
    return 1;
  }

  debugf("Bitmap size in bytes: %d\n", BitmapSizeInBytes);

  Bitmap = (uint32_t *)(mmmt->addr);

  //    for (int i = 0; i < BitmapSizeInBlocks; i++) Bitmap[i] = 0xffffffff;
  for (int i = 0; i < BitmapSizeInBlocks; i++)
    Set(i, true);

  //    MarkBlocks(0x0, 4, false);
  // MarkRegion((void *)(mem_start + 0x0), BLOCK_SIZE * 64, false);

  // for (int i = 0; i < memoryMapCnt; i++) {
  //   mmmt = memoryMap[i];
  //   if (mmmt->addr > mbi_memorySize)
  //     continue;
  //   if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE) {
  //     MarkBlocks(ToBlockRoundUp(mmmt->addr), mmmt->len / BLOCK_SIZE, 0);
  //   }
  // }

  // for (int i = 0; i < memoryMapCnt; i++) {
  //   mmmt = memoryMap[i];
  //   if (mmmt->addr > mbi_memorySize)
  //     continue;
  //   if (mmmt->type != MULTIBOOT_MEMORY_AVAILABLE)
  //     MarkBlocks(ToBlock(mmmt->addr), DivRoundUp(mmmt->len, BLOCK_SIZE), 1);
  // }

  uint64_t best_start = 0;
  uint64_t best_len = 0;
  for (int i = 0; i < memoryMapCnt; i++) {
    mmmt = memoryMap[i];
    if (mmmt->addr > mbi_memorySize)
      continue;
    if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE && mmmt->len > best_len) {
      best_start = mmmt->addr;
      best_len = mmmt->len;
    }
  }

  MarkBlocks(ToBlockRoundUp(best_start), best_len / BLOCK_SIZE, 0);

  MarkRegion(Bitmap, BitmapSizeInBytes, 1);
}
