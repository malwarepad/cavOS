#include <disk.h>
#include <fat32.h>
#include <liballoc.h>
#include <system.h>

// FAT32's "(L)ong (F)ile(N)ame" entry parsing
// Copyright (C) 2024 Panagiotis

bool lfnCmp(FAT32 *fat, uint32_t clusterNum, uint16_t nthOf32, char *str) {
#if FAT32_DBG_PROMPTS
  debugf("[fat32::lfn::compare] Searching against clusterNum{%d} nthOf32{%d} "
         "str{%s}...\n",
         clusterNum, nthOf32, str);
#endif
  uint16_t *lfn = calcLfn(fat, clusterNum, nthOf32);
  uint32_t  lfnlen = 0;
  uint32_t  strlen = 0;
  while (lfn[lfnlen] != '\0')
    lfnlen++;
  while (str[strlen] != '\0')
    strlen++;
#if FAT32_DBG_PROMPTS
  debugf("[fat32::lfn::compare] Comparing with firstByte{%x} lfnlen{%d} "
         "strlen{%d} "
         "[cluster:%d nth=%d]\n\n",
         lfn[0], lfnlen, strlen, clusterNum, nthOf32);
#endif
  if (lfnlen != strlen)
    return false;
  for (uint32_t i = 0; lfn[i] != '\0'; i++) {
    if (str[i] == '\0' || lfn[i] != str[i]) {
      free(lfn);
      return false;
    }
  }
  free(lfn);
  return true;
}

bool isLFNentry(FAT32 *fat, uint8_t *rawArr, uint32_t clusterNum,
                uint16_t entry) {
  if (entry == 0) { // first entry of cluster
    uint8_t *newRawArr =
        (uint8_t *)malloc(fat->sectors_per_cluster * SECTOR_SIZE);
    uint32_t lba = fat->cluster_begin_lba +
                   (clusterNum - 2 - 1) * fat->sectors_per_cluster;
    getDiskBytes(newRawArr, lba, fat->sectors_per_cluster);
    pFAT32_Directory dir =
        (pFAT32_Directory)((uint32_t)newRawArr +
                           fat->sectors_per_cluster * SECTOR_SIZE - 32);
    bool res = dir->attributes == 0x0F;
#if FAT32_DBG_PROMPTS
    debugf("[fat32::lfn::check] Completed LFS entry check: result{%d} "
           "attributes{%x}!\n",
           res, dir->attributes);
#endif
    free(newRawArr);
    return res;
  }

  pFAT32_Directory dir =
      (pFAT32_Directory)((uint32_t)rawArr + (entry - 1) * 32);
  return dir->attributes == 0x0F;
}

uint16_t *calcLfn(FAT32 *fat, uint32_t clusterNum, uint16_t nthOf32) {
  if (clusterNum < 2)
    return 0;

  uint8_t *rawArr = (uint8_t *)malloc(fat->sectors_per_cluster * SECTOR_SIZE);
  uint32_t currCluster = 0;
  // todo: remove limit of 512 chars per .../{filename}/...
  uint16_t *filename = (uint16_t *)malloc(512 * 2);
  uint32_t  currFilenamePos = 0;
  if (nthOf32 == 0) { // first entry in cluster
    currCluster = 1;
    nthOf32 = 33;
  }
  while (1) {
    uint32_t lba = fat->cluster_begin_lba +
                   (clusterNum - 2 - currCluster) * fat->sectors_per_cluster;
#if FAT32_DBG_PROMPTS
    debugf("[fat32::lfn::calculate] Calculating LFN full filename: lba{%d} "
           "nthOf32{%d} clusterNum{%d} "
           "currCluster{%d}\n",
           lba, nthOf32, clusterNum, currCluster);
#endif
    getDiskBytes(rawArr, lba, fat->sectors_per_cluster);
    for (uint16_t i = (nthOf32 - 1); i >= 0; i--) {
      if (i == 0) { // last entry in cluster
        // i = -1;
        nthOf32 = 33;
        currCluster++;
      }
      FAT32_LFN *lfn = (FAT32_LFN *)((uint32_t)rawArr + 32 * i);

#if FAT32_DBG_PROMPTS
      debugf("[fat32::lfn::calculate] [%x:%x] ", lba * 512 + 32 * i,
             (fat->cluster_begin_lba +
              (clusterNum - 2) * fat->sectors_per_cluster) *
                 (fat->sectors_per_cluster * SECTOR_SIZE));
      for (int i = 0; i < 32; i++) {
        debugf("%02x ", ((uint8_t *)lfn)[i]);
      }
      debugf("\n[fat32::lfn::calculate] order{%d}\n", lfn->order);
#endif
      if (lfn->attributes != 0x0F) {
        // printf("end of long \n", i);
        filename[currFilenamePos++] = '\0';
        free(rawArr);
        return filename; // has to be free()'d later
      }

      for (uint8_t j = 0; j < 5; j++)
        filenameAssign(filename, &currFilenamePos, lfn->char_sequence_1[j]);

      for (uint8_t j = 0; j < 6; j++)
        filenameAssign(filename, &currFilenamePos, lfn->char_sequence_2[j]);

      for (uint8_t j = 0; j < 2; j++)
        filenameAssign(filename, &currFilenamePos, lfn->char_sequence_3[j]);

#if FAT32_DBG_PROMPTS
      debugf("\n\n");
#endif

      // if ((rawArr[32 * i] & 0x40) != 0) { // last long
      //   printf(" last long detected [index=%d] \n", i);
      // } else { // any long
      //   printf(" nth long detected [index=%d] \n", i);
      // }
    }
  }

  free(rawArr);
  free(filename);

  return 0;
}