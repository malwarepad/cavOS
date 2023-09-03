#include "../../include/fat32.h"

// Simple alpha FAT32 driver according to the Microsoft specification
// Copyright (C) 2023 Panagiotis

#define FAT32_PARTITION_OFFSET_LBA 2048 // 1048576, 1MB

int initiateFat32() {
  printf("\n[+] FAT32: Initializing...");

  printf("\n[+] FAT32: Reading disk0 at lba %d...", FAT32_PARTITION_OFFSET_LBA);
  unsigned char rawArr[SECTOR_SIZE];
  getDiskBytes(rawArr, FAT32_PARTITION_OFFSET_LBA, 1);

  printf("\n[+] FAT32: Checking if disk0 at lba %d is FAT32 formatted...",
         FAT32_PARTITION_OFFSET_LBA);
  // get sector count
  const unsigned char littleSectorCount =
      rawArr[19] | (rawArr[20] << 8); // (two, little endian)
  int bigSectorCount = 0;
  for (int i = 3; i >= 0; i--) {
    bigSectorCount =
        (bigSectorCount << 8) | rawArr[32 + i]; // (four, little endian)
  }
  fat.sector_count =
      littleSectorCount == 0 ? bigSectorCount : littleSectorCount;

  // get FAT's
  fat.number_of_fat = rawArr[16];

  // get reserved sectors (two, little endian)
  fat.reserved_sectors = rawArr[14] | (rawArr[15] << 8);

  // get sectors / track (two, little endian)
  fat.sectors_per_track = rawArr[24] | (rawArr[25] << 8);

  // get sectors / cluster
  fat.sectors_per_cluster = rawArr[13];

  // get sectors / fat (four, little endian)
  fat.sectors_per_fat = 0;
  for (int i = 3; i >= 0; i--) {
    fat.sectors_per_fat = (fat.sectors_per_fat << 8) | rawArr[36 + i];
  }

  // volume id
  fat.volume_id = 0;
  for (int i = 0; i < 4; i++) {
    fat.volume_id = (fat.volume_id << 8) | rawArr[67 + i];
  }

  // volume name
  for (int i = 0; i < 11; i++) {
    fat.volume_label[i] = (char)rawArr[71 + i];
  }
  fat.volume_label[11] = '\0';

  if (fat.sector_count == 0 || fat.reserved_sectors == 0 ||
      fat.sectors_per_track == 0 || fat.volume_id == 0) {
    printf("\n[+] FAT32: Failed to parse FAT information... This kernel only "
           "supports FAT32!\n");
    return 0;
  }
  if (rawArr[66] != 0x28 && rawArr[66] != 0x29) {
    printf("\n[+] FAT32: Incorrect disk signature! This kernel only supports "
           "FAT32!\n");
    return 0;
  }

  fat.fat_begin_lba = FAT32_PARTITION_OFFSET_LBA + fat.reserved_sectors;
  fat.cluster_begin_lba = FAT32_PARTITION_OFFSET_LBA + fat.reserved_sectors +
                          (fat.number_of_fat * fat.sectors_per_fat);

  fat.works = 1;

  printf("\n[+] FAT32: Valid FAT32 formatted drive: [%X] %s", fat.volume_id,
         fat.volume_label);
  printf("\n    [+] Sector count: %d", fat.sector_count);
  printf("\n    [+] FAT's: %d", fat.number_of_fat);
  printf("\n    [+] Reserved sectors: %d", fat.reserved_sectors);
  printf("\n    [+] Sectors / FAT: %d", fat.sectors_per_fat);
  printf("\n    [+] Sectors / track: %d", fat.sectors_per_track);
  printf("\n    [+] Sectors / cluster: %d", fat.sectors_per_cluster);
  printf("\n");
}

int highLowCombiner(uint16_t highBits[2], uint16_t lowBits[2]) {
  uint32_t clusterNumber[2];

  int cluster[2];

  for (int i = 0; i < 2; i++) {
    cluster[i] = ((int)highBits[i]) << 16 | (int)lowBits[i];
  }

  return cluster;
}

unsigned int getFatEntry(int cluster) {
  int lba = fat.fat_begin_lba + (cluster * 4 / SECTOR_SIZE);
  int entryOffset = (cluster * 4) % SECTOR_SIZE;

  unsigned char *rawArr = (unsigned char *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, lba, 1);
  // unsigned int c = *(uint32 *)&rawArr[entryOffset] & 0x0FFFFFFF; //
  // 0x0FFFFFFF mask to keep lower 28 bits valid, nah fuck this ima do it
  // manually, watch & learn
  if (rawArr[entryOffset] >= 0xFFF8)
    return 0;

  unsigned int result = 0;
  for (int i = 3; i >= 0; i--) {
    result = (result << 8) | rawArr[entryOffset + i];
  }

  // printf("\n[%d] %x %x %x %x {Binary: %d Hexadecimal: %x}\n", entryOffset,
  //        rawArr[entryOffset], rawArr[entryOffset + 1], rawArr[entryOffset +
  //        2], rawArr[entryOffset + 3], result, result);

  return result;
}

int showCluster(int clusterNum, int attrLimitation) // NOT 0, NOT 1
{
  if (clusterNum < 2)
    return 0;

  const int lba =
      fat.cluster_begin_lba + (clusterNum - 2) * fat.sectors_per_cluster;
  unsigned char *rawArr = (unsigned char *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, lba, 1);

  for (int i = 0; i < (SECTOR_SIZE / 32); i++) {
    if (rawArr[32 * i] == 0)
      break;

    if (attrLimitation != NULL && rawArr[32 * i + 11] != attrLimitation)
      continue;

    uint8 attr = rawArr[32 * i + 11];
    uint8 reserved = rawArr[32 * i + 12];

    unsigned int createdDate = (rawArr[32 * i + 17] << 8) | rawArr[32 * i + 16];
    int          createdDay = createdDate & 0x1F;
    int          createdMonth = (createdDate >> 5) & 0xF;
    int          createdYear = ((createdDate >> 9) & 0x7F) + 1980;

    printf("[%d] attr: 0x%02X | created: %02d/%02d/%04d | ", reserved, attr,
           createdDay, createdMonth, createdYear);
    for (int o = 0; o < 11; o++) {
      printf("%c", rawArr[32 * i + o]);
    }
    printf("\n");
  }

  if (rawArr[SECTOR_SIZE - 32] != 0) {
    unsigned int nextCluster = getFatEntry(clusterNum);
    if (nextCluster == 0)
      return;
    showCluster(nextCluster, attrLimitation);
  }

  return 1;
}

int showFileByCluster(int clusterNum, int size) {
  clearScreen();
  for (int i = 0; i < DIV_ROUND_CLOSEST(size, SECTOR_SIZE); i++) { // 1
    unsigned char *rawArr = (unsigned char *)malloc(SECTOR_SIZE);
    const int      lba =
        fat.cluster_begin_lba + (clusterNum - 2) * fat.sectors_per_cluster + i;
    getDiskBytes(rawArr, lba, 1);
    for (int j = 0; j < SECTOR_SIZE; j++)
      printf("%c", rawArr[j]);
  }
}