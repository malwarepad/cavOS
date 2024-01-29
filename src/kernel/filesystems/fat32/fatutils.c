#include <disk.h>
#include <fat32.h>
#include <liballoc.h>
#include <system.h>

// Various utilities for the FAT32's filesystem
// Copyright (C) 2023 Panagiotis

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

void filenameAssign(uint16_t *filename, uint32_t *currFilenamePos,
                    uint16_t character) {
#if FAT32_DBG_PROMPTS
  debugf("%c", character);
#endif
  if (character != 0 && character != 0xffff)
    filename[(*currFilenamePos)++] = character;
}

// --- Debugging interfaces below --- //

void fileReaderTest(FAT32 *fat) {
  if (!fat->works) {
    printf("\nFAT32 was not initalized properly on boot!\n");
    return;
  }
  clearScreen();
  printf("=========================================\n");
  printf("====      cavOS file reader 1.0      ====\n");
  printf("====    Copyright MalwarePad 2023    ====\n");
  printf("=========================================\n");

  printf("\nFile path + filename (i.e. /untitled.txt):\n> ");
  char *choice = (char *)malloc(200);
  readStr(choice);
  printf("\n");

  FAT32_Directory dir;
  if (!fatOpenFile(fat, &dir, choice)) {
    printf("Cannot find file!\n");
    return;
  }

  // showFile(&dir);
  char *out = (char *)malloc(dir.filesize);
  readFileContents(fat, &out, &dir);
  printf("%s", out);
  free(out);
  printf("\n\n");
}

// todo, redo this crap
bool showCluster(FAT32 *fat, uint32_t clusterNum,
                 uint8_t attrLimitation) // NOT 0, NOT 1
{
  if (clusterNum < 2)
    return 0;

  const int lba =
      fat->cluster_begin_lba + (clusterNum - 2) * fat->sectors_per_cluster;
  uint8_t *rawArr = (uint8_t *)malloc(fat->sectors_per_cluster * SECTOR_SIZE);
  getDiskBytes(rawArr, lba, fat->sectors_per_cluster);

  for (int i = 0; i < ((fat->sectors_per_cluster * SECTOR_SIZE) / 32); i++) {
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
    bool lfn = false;
    lfn = isLFNentry(fat, rawArr, clusterNum, i);
    printf("%s", directory->filename);
    printf("\n");
    if (lfn) {
      uint16_t *str = calcLfn(fat, clusterNum, i);
      for (int i = 0; str[i] != '\0'; i++) {
        printf("%c", str[i]);
      }
      free(str);
    }
    printf("\n");
  }

  if (rawArr[fat->sectors_per_cluster * SECTOR_SIZE - 32] != 0) {
    unsigned int nextCluster = getFatEntry(fat, clusterNum);
    if (nextCluster == 0)
      return 1;
    showCluster(fat, nextCluster, attrLimitation);
  }

  free(rawArr);

  return 1;
}
