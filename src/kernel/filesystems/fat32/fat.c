#include "../../include/disk.h"
#include "../../include/fat32.h"
#include "../../include/liballoc.h"
#include "../../include/system.h"

// NOTE: This file stands for FAT32's "(F)ile (A)llocation (T)able" and is NOT
// the entry to the fat32 filesystem!

// File Allocation Table parsing/manipulation functions
// Copyright (C) 2023 Panagiotis

uint32_t getFatEntry(uint32_t cluster) {
  uint32_t lba = fat->fat_begin_lba + (cluster * 4 / SECTOR_SIZE);
  uint32_t entryOffset = (cluster * 4) % SECTOR_SIZE;

  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, lba, 1);

  uint32_t result = *((uint32_t *)(&rawArr[entryOffset]));

  result &= 0x0FFFFFFF;

  free(rawArr);

  if (result >= 0x0FFFFFF8)
    return 0;

  return result;
}

void setFatEntry(uint32_t cluster, uint32_t value) {
  uint32_t lba = fat->fat_begin_lba + (cluster * 4 / SECTOR_SIZE);
  uint32_t entryOffset = (cluster * 4) % SECTOR_SIZE;

  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, lba, 1);

  uint32_t *rawArr32 = (uint32_t)rawArr;
  rawArr32[entryOffset / 4] = value;

  write_sectors_ATA_PIO(lba, 1, rawArr32);
  free(rawArr);
}
