#include "../../include/fat32.h"
#include "../../include/ata.h"
#include "../../include/system.h"

// Simple alpha FAT32 driver according to the Microsoft specification
// Copyright (C) 2023 Panagiotis

// #define FAT32_PARTITION_OFFSET_LBA 2048 // 1048576, 1MB

// todo. allow multiple FAT32* structures by implicitly passing them to
// todo. functions instead of using a global one declared at compile time

bool initiateFat32(uint32_t disk, uint8_t partition_num) {
  fat = (FAT32 *)malloc(sizeof(FAT32));
  fatinitf("[+] FAT32: Initializing...");

  mbr_partition *partPtr = (mbr_partition *)malloc(sizeof(mbr_partition));
  openDisk(disk, partition_num, partPtr);
  fat->partition = partPtr;

  fatinitf("\n[+] FAT32: Reading disk0 at lba %d...",
           fat->partition->lba_first_sector);
  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, fat->partition->lba_first_sector, 1);

  fatinitf("\n[+] FAT32: Checking if disk0 at lba %d is FAT32 formatted...",
           fat->partition->lba_first_sector);

  fat = (FAT32 *)rawArr;

  if (fat->sector_count == 0 || fat->reserved_sectors == 0 ||
      fat->sectors_per_track == 0 || fat->volume_id == 0) {
    fatinitf("\n[+] FAT32: Failed to parse FAT information... This kernel only "
             "supports FAT32!\n");
    return false;
  }
  if (rawArr[66] != FAT_SIGNATURE1 && rawArr[66] != FAT_SIGNATURE2) {
    fatinitf("\n[+] FAT32: Incorrect disk signature! This kernel only supports "
             "FAT32!\n");
    return false;
  }

  fat->partition =
      partPtr; // at "fat = (FAT32 *)rawArr;" we already overwrote the main ptr
  fat->fat_begin_lba = fat->partition->lba_first_sector + fat->reserved_sectors;
  fat->cluster_begin_lba = fat->partition->lba_first_sector +
                           fat->reserved_sectors +
                           (fat->number_of_fat * fat->sectors_per_fat);

  fat->works = true;

  fatinitf("\n[+] FAT32: Valid FAT32 formatted drive: [%X] %.11s",
           fat->volume_id, fat->volume_label);
  fatinitf("\n    [+] Sector count: %d", fat->sector_count);
  fatinitf("\n    [+] FAT's: %d", fat->number_of_fat);
  fatinitf("\n    [+] Reserved sectors: %d", fat->reserved_sectors);
  fatinitf("\n    [+] Sectors / FAT: %d", fat->sectors_per_fat);
  fatinitf("\n    [+] Sectors / track: %d", fat->sectors_per_track);
  fatinitf("\n    [+] Sectors / cluster: %d", fat->sectors_per_cluster);
  fatinitf("\n");

  // free(rawArr);

  return true;
}

bool findFile(pFAT32_Directory fatdir, uint32_t initialCluster,
              char *filename) {
  uint32_t clusterNum = initialCluster;
  uint8_t *rawArr = (uint8_t *)malloc(fat->sectors_per_cluster * SECTOR_SIZE);
  while (1) {
    uint32_t lba =
        fat->cluster_begin_lba + (clusterNum - 2) * fat->sectors_per_cluster;
    getDiskBytes(rawArr, lba, fat->sectors_per_cluster);

    for (uint16_t i = 0; i < ((fat->sectors_per_cluster * SECTOR_SIZE) / 32);
         i++) {
      if (rawArr[32 * i] == FAT_DELETED || rawArr[32 * i + 11] == 0x0F)
        continue;
#if FAT32_DBG_PROMPTS
      debugf("[search] clusterNum=%d i=%d fc=%c\n", clusterNum, i,
             rawArr[32 * i]);
#endif
      if (isLFNentry(rawArr, clusterNum, i)
              ? lfnCmp(clusterNum, i, filename)
              : (memcmp(rawArr + (32 * i), formatToShort8_3Format(filename),
                        11) == 0)) {
        *fatdir = *(pFAT32_Directory)(&rawArr[32 * i]);
        fatdir->lba = lba;
        fatdir->currEntry = i;
#if FAT32_DBG_PROMPTS
        debugf("[search] filename: %s\n", filename);
        debugf("[search] low=%d low1=%x low2=%x\n", (*fatdir).firstClusterLow,
               rawArr[32 * i + 26], rawArr[32 * i + 27]);
        for (int o = 0; o < 32; o++) {
          debugf("%x ", rawArr[32 * i + o]);
        }
        debugf("\n");
#endif
        free(rawArr);
        return true;
      }
    }

    if (rawArr[fat->sectors_per_cluster * SECTOR_SIZE - 32] != 0) {
      uint32_t nextCluster = getFatEntry(clusterNum);
      if (nextCluster == 0) {
        memset(fatdir, 0, sizeof(FAT32_Directory));
        free(rawArr);
        return false;
      }
      clusterNum = nextCluster;
    } else {
      memset(fatdir, 0, sizeof(FAT32_Directory));
      free(rawArr);
      return false;
    }
  }
}

void readFileContents(char **rawOut, pFAT32_Directory dir) {
#if FAT32_DBG_PROMPTS
  debugf("[read] filesize=%d cluster=%d\n", dir->filesize,
         dir->firstClusterLow);
#endif
  if (dir->attributes != 0x20) {
    debugf("[read] Seriously tried to read non-file entry (0x%02X attr)\n",
           dir->attributes);
    return;
  }

  char    *out = *rawOut;
  uint32_t curr = 0;
  uint32_t currCluster = dir->firstClusterLow;
  uint8_t *rawArr = (uint8_t *)malloc(fat->sectors_per_cluster * SECTOR_SIZE);
  while (1) { // DIV_ROUND_CLOSEST(dir->filesize, SECTOR_SIZE)
    uint32_t lba =
        fat->cluster_begin_lba + (currCluster - 2) * fat->sectors_per_cluster;
    getDiskBytes(rawArr, lba, fat->sectors_per_cluster);
#if FAT32_DBG_PROMPTS
    debugf("[read] next one: 0x%x %d\n", currCluster, currCluster);
#endif
    for (uint16_t j = 0; j < fat->sectors_per_cluster * SECTOR_SIZE; j++) {
      if (curr > dir->filesize) {
#if FAT32_DBG_PROMPTS
        debugf("[read] reached filesize end (limit) at: 0x%x %d\n", currCluster,
               currCluster);
#endif
        free(rawArr);
        return;
      }
      // #if FAT32_DBG_PROMPTS
      // debugf("%c", rawArr[j]);
      // #endif
      out[curr] = rawArr[j];
      curr++;
    }
    uint32_t old = currCluster;
    currCluster = getFatEntry(old);
    if (currCluster == 0) {
#if FAT32_DBG_PROMPTS
      debugf("[read] reached hard end (0x0), showing last cluster: 0x%x %d\n",
             old, old);
#endif
      break;
    }
  }
  free(rawArr);
}

bool openFile(pFAT32_Directory dir, char *filename) {
  if (filename[0] != '/')
    return 0;

  uint32_t index = 0;
  uint32_t len = strlength(filename);
  char    *tmpBuff = (char *)malloc(len + 1);
  dir->firstClusterLow = 2;
  memset(tmpBuff, '\0', len);

  for (uint32_t i = 1; i < len; i++) { // skip index 0
    if (filename[i] == '/' || (i + 1) == len) {
      if ((i + 1) == len)
        tmpBuff[index++] = filename[i];
      if (!findFile(dir, dir->firstClusterLow, tmpBuff)) {
        debugf("[openFile] Could not open file %s!\n", filename);
        free(tmpBuff);
        return false;
      }

      // cleanup
      memset(tmpBuff, '\0', len);
      index = 0;
    } else
      tmpBuff[index++] = filename[i];
  }

  free(tmpBuff);
  return true;
}

bool deleteFile(char *filename) {
  FAT32_Directory fatdir;
  if (!openFile(&fatdir, filename))
    return false;
  uint8_t *rawArr = (uint8_t *)malloc(fat->sectors_per_cluster * SECTOR_SIZE);
  getDiskBytes(rawArr, fatdir.lba, fat->sectors_per_cluster);
  rawArr[32 * fatdir.currEntry] = FAT_DELETED;
  write_sectors_ATA_PIO(fatdir.lba, fat->sectors_per_cluster, rawArr);

  // invalidate
  uint32_t currCluster = fatdir.firstClusterLow;
  while (1) {
    setFatEntry(currCluster, 0);
    uint32_t old = currCluster;
    currCluster = getFatEntry(old);
    if (currCluster == 0)
      break;
  }

  return true;
}
