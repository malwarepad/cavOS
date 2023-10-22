#include "../../include/fat32.h"

// Simple alpha FAT32 driver according to the Microsoft specification
// Copyright (C) 2023 Panagiotis

// #define FAT32_PARTITION_OFFSET_LBA 2048 // 1048576, 1MB

// todo. allow multiple FAT32* structures by implicitly passing them to
// todo. functions instead of using a global one declared at compile time

#define FAT32_DBG_PROMPTS 0
#define fatinitf debugf

int initiateFat32(uint32_t disk, uint8_t partition_num) {
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
    return 0;
  }
  if (rawArr[66] != FAT_SIGNATURE1 && rawArr[66] != FAT_SIGNATURE2) {
    fatinitf("\n[+] FAT32: Incorrect disk signature! This kernel only supports "
             "FAT32!\n");
    return 0;
  }

  fat->partition =
      partPtr; // at "fat = (FAT32 *)rawArr;" we already overwrote the main ptr
  fat->fat_begin_lba = fat->partition->lba_first_sector + fat->reserved_sectors;
  fat->cluster_begin_lba = fat->partition->lba_first_sector +
                           fat->reserved_sectors +
                           (fat->number_of_fat * fat->sectors_per_fat);

  fat->works = 1;

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

  return 1;
}

unsigned int getFatEntry(int cluster) {
  int lba = fat->fat_begin_lba + (cluster * 4 / SECTOR_SIZE);
  int entryOffset = (cluster * 4) % SECTOR_SIZE;

  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
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

  free(rawArr);

  return result;
}

int findFile(pFAT32_Directory fatdir, int initialCluster, char *filename) {
  int     clusterNum = initialCluster;
  uint8_t rawArr[SECTOR_SIZE];
  while (1) {
    const int lba =
        fat->cluster_begin_lba + (clusterNum - 2) * fat->sectors_per_cluster;
    getDiskBytes(rawArr, lba, 1);

    for (int i = 0; i < (SECTOR_SIZE / 32); i++) {
      if (memcmp(rawArr + (32 * i), filename, 11) == 0 &&
          rawArr[32 * i] != FAT_DELETED) { // fatdir->filename
        *fatdir = *(pFAT32_Directory)(&rawArr[32 * i]);
        fatdir->lba = lba;
#if UNSAFE_DBG
        debugf("[search] filename: %s\n", filename);
        debugf("[search] low=%d low1=%x low2=%x\n", (*fatdir).firstClusterLow,
               rawArr[32 * i + 26], rawArr[32 * i + 27]);
        for (int o = 0; o < 32; o++) {
          debugf("%x ", rawArr[32 * i + o]);
        }
        debugf("\n");
#endif
        return 1;
      }
    }

    if (rawArr[SECTOR_SIZE - 32] != 0) {
      unsigned int nextCluster = getFatEntry(clusterNum);
      if (nextCluster == 0)
        return 0;
      clusterNum = nextCluster;
    } else
      return 0;
  }
}

char *formatToShort8_3Format(char *directory) {
  static char out[12]; // 8 characters + dot + 3 characters + null terminator
  int         i;

  for (i = 0; i < 11; i++) {
    out[i] = ' ';
  }
  out[11] = '\0';

  int len = strlength(directory);
  int dotIndex = -1;

  for (i = len - 1; i >= 0; i--) {
    if (directory[i] == '.') {
      dotIndex = i;
      break;
    }
  }

  int nameLength = (dotIndex == -1) ? len : dotIndex;
  for (i = 0; i < nameLength && i < 8; i++) {
    char c = directory[i];
    if (c >= 'a' && c <= 'z') {
      c -= 'a' - 'A';
    }
    out[i] = c;
  }

  if (dotIndex != -1) {
    int extensionLength = len - (dotIndex + 1);
    for (i = 0; i < extensionLength && i < 3; i++) {
      char c = directory[dotIndex + 1 + i];
      if (c >= 'a' && c <= 'z') {
        c -= 'a' - 'A';
      }
      out[8 + i] = c;
    }
  }

  return out;
}

int charPrintNoFF(char *target) {
  if (*target != 0xFFFFFFFF)
    printf("%c", *target);
}

int calcLfn(int clusterNum, int nthOf32) {
  if (clusterNum < 2)
    return 0;

  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  int      curr = 0;

  if (nthOf32 == 0) {
    curr = 1;
    nthOf32 = 33;
  }
  int checksum = 0x0;
  while (1) {
    const int lba = fat->cluster_begin_lba +
                    (clusterNum - 2 - curr) * fat->sectors_per_cluster;
    getDiskBytes(rawArr, lba, 1);
    for (int i = (nthOf32 - 1); i >= 0; i--) {
      if (rawArr[32 * i + 11] != 0x0F) {
        // printf("end of long \n", i);
        // printf(" | checksum: 0x%lx\n", checksum);
        printf("\n");
        return 1;
      }

      if (checksum == 0x0)
        printf("    [-1] ");

      for (int j = 0; j < 5; j++) {
        charPrintNoFF(&rawArr[32 * i + 1 + (j * 2)]);
      }

      for (int j = 0; j < 6; j++) {
        charPrintNoFF(&rawArr[32 * i + 14 + (j * 2)]);
      }

      for (int j = 0; j < 2; j++) {
        charPrintNoFF(&rawArr[32 * i + 28 + (j * 2)]);
      }

      checksum = rawArr[32 * i + 13];

      // if ((rawArr[32 * i] & 0x40) != 0) { // last long
      //   printf(" last long detected [index=%d] \n", i);
      // } else { // any long
      //   printf(" nth long detected [index=%d] \n", i);
      // }
    }
  }

  free(rawArr);

  return 0;
}

int showCluster(int clusterNum, int attrLimitation) // NOT 0, NOT 1
{
  if (clusterNum < 2)
    return 0;

  const int lba =
      fat->cluster_begin_lba + (clusterNum - 2) * fat->sectors_per_cluster;
  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, lba, 1);

  for (int i = 0; i < (SECTOR_SIZE / 32); i++) {
    if (rawArr[32 * i] == 0)
      break;

    pFAT32_Directory directory = (pFAT32_Directory)(&rawArr[32 * i]);

    if ((directory->attributes == 0x0F) || (directory->attributes == 0x08) ||
        rawArr[32 * i] == FAT_DELETED ||
        (attrLimitation != NULL && directory->attributes != attrLimitation))
      continue;

    uint16_t createdDate = directory->creationDate;
    int      createdDay = createdDate & 0x1F;
    int      createdMonth = (createdDate >> 5) & 0xF;
    int      createdYear = ((createdDate >> 9) & 0x7F) + 1980;

    printf("[%d] attr: 0x%2X | created: %2d/%2d/%4d | ", directory->ntReserved,
           directory->attributes, createdDay, createdMonth, createdYear);
    int lfn = 0;
    // char *out;
    for (int o = 0; o < 11; o++) {
      if (rawArr[32 * i + o] == '~')
        lfn = 1;
      // out[o] = rawArr[32 * i + o];
      // printf("%c", rawArr[32 * i + o]);
    }
    // out[11] = '\0';
    // formatFilename(&directory->filename);
    printf("%s", directory->filename);
    printf("\n");
    if (lfn)
      calcLfn(clusterNum, i);
  }

  if (rawArr[SECTOR_SIZE - 32] != 0) {
    unsigned int nextCluster = getFatEntry(clusterNum);
    if (nextCluster == 0)
      return 1;
    showCluster(nextCluster, attrLimitation);
  }

  free(rawArr);

  return 1;
}

int divisionRoundUp(int a, int b) { return (a + (b - 1)) / b; }

void readFileContents(char **rawOut, pFAT32_Directory dir) {
#if UNSAFE_DBG
  debugf("[read] filesize=%d cluster=%d\n", dir->filesize,
         dir->firstClusterLow);
#endif
  char *out = *rawOut;
  int   curr = 0;
  for (int i = 0; i < divisionRoundUp(dir->filesize, SECTOR_SIZE);
       i++) { // DIV_ROUND_CLOSEST(dir->filesize, SECTOR_SIZE)
    unsigned char rawArr[SECTOR_SIZE];
    const int     lba = fat->cluster_begin_lba +
                    (dir->firstClusterLow - 2) * fat->sectors_per_cluster + i;
    getDiskBytes(rawArr, lba, 1);
    for (int j = 0; j < SECTOR_SIZE; j++) {
      if (curr > dir->filesize)
        return;
      out[curr] = rawArr[j];
      curr++;
    }
  }
  return;
}

int showFile(pFAT32_Directory dir) {
#if UNSAFE_DBG
  debugf("[read] filesize=%d cluster=%d\n", dir->filesize,
         dir->firstClusterLow);
#endif
  for (int i = 0; i < divisionRoundUp(dir->filesize, SECTOR_SIZE);
       i++) { // DIV_ROUND_CLOSEST(dir->filesize, SECTOR_SIZE)
    unsigned char rawArr[SECTOR_SIZE];
    const int     lba = fat->cluster_begin_lba +
                    (dir->firstClusterLow - 2) * fat->sectors_per_cluster + i;
    getDiskBytes(rawArr, lba, 1);
    for (int j = 0; j < SECTOR_SIZE; j++)
      printf("%c", rawArr[j]);
  }
}

int openFile(pFAT32_Directory dir, char *filename) {
  if (filename[0] != '/')
    return 0;

  int   index = 0;
  int   len = strlength(filename);
  char *tmpBuff = (char *)malloc(len + 1);
  dir->firstClusterLow = 2;
  memset(tmpBuff, '\0', len);

  for (int i = 1; i < len; i++) { // skip index 0
    if (filename[i] == '/' || (i + 1) == len) {
      if ((i + 1) == len)
        tmpBuff[index++] = filename[i];
      if (!findFile(dir, dir->firstClusterLow,
                    formatToShort8_3Format(tmpBuff))) {
        free(tmpBuff);
        return 0;
      }

      // cleanup
      memset(tmpBuff, '\0', len);
      index = 0;
    } else
      tmpBuff[index++] = filename[i];
  }

  free(tmpBuff);
  return 1;
}

int fileReaderTest() {
  clearScreen();
  printf("=========================================\n");
  printf("====      cavOS file reader 1.0      ====\n");
  printf("====    Copyright MalwarePad 2023    ====\n");
  printf("=========================================\n");

  printf("\nEnter cluster choice (2 -> root directory):\n> ");
  char *choice = (char *)malloc(200);
  readStr(choice);
  int cluster = atoi(choice);
  free(choice);
  printf("\n\nCluster information:\n");
  showCluster(cluster, 0);
  printf("\n");

  while (1) {
    printf("Note: } can be used to exit...\nEnter target filename (8.3 "
           "short):\n> ");
    char cnt[200];
    readStr(cnt);
    printf("\n");

    if (cnt[0] == '}')
      return 1;

    char *res = &cnt;
#if UNSAFE_DBG
    debugf("[input]: %s\n", res);
#endif
    char *modifiable = formatToShort8_3Format(res);
    // for (int i = 0; i < 11; i++) {
    //   // printf("%2x ", modifiable[i]);
    // }
#if UNSAFE_DBG
    debugf("[parse] FAT32-compatible filename: %s\n", modifiable);
#endif

    FAT32_Directory dir;
    findFile(&dir, cluster, modifiable);
    if (dir.filename[0] == 0x10) {
      printf("\nNo such file can be found!\n[input: %s] [prased: %s]\n\n", res,
             modifiable);
      continue;
    }

#if UNSAFE_DBG
    debugf("[search res] filename=%s attr=0x%x low=%d\n", dir.filename,
           dir.attributes, dir.firstClusterLow);
#endif

    // showFile(&dir);
    char *out = (char *)malloc(dir.filesize);
    readFileContents(&out, &dir);
    printf("%s", out);
    free(out);
    printf("\n\n");
  }
}

int deleteFile(char *filename) {
  FAT32_Directory fatdir;
  if (!openFile(&fatdir, filename))
    return 0;
  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, fatdir.lba, 1);
  for (int i = 0; i < (SECTOR_SIZE / 32); i++) {
    if (memcmp(rawArr + (32 * i), fatdir.filename, 11) == 0) {
      rawArr[32 * i] = FAT_DELETED;
      write_sectors_ATA_PIO(fatdir.lba, 1, rawArr);
      break;
    }
  }

  return 1;
}
