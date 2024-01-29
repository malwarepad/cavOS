#include <disk.h>
#include <util.h>

// Multiple disk handler
// Copyright (C) 2022 Panagiotis

uint16_t mbr_partition_indexes[] = {MBR_PARTITION_1, MBR_PARTITION_2,
                                    MBR_PARTITION_3, MBR_PARTITION_4};

bool openDisk(uint32_t disk, uint8_t partition, mbr_partition *out) {
  disk = disk; // future plans

  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  read_sectors_ATA_PIO(rawArr, 0x0, 1);
  // *out = *(mbr_partition *)(&rawArr[mbr_partition_indexes[partition]]);
  bool ret = validateMbr(rawArr);
  if (!ret)
    return false;
  memcpy(out, (void *)((uint32_t)rawArr + mbr_partition_indexes[partition]),
         sizeof(mbr_partition));
  free(rawArr);
  return true;
}

bool validateMbr(uint8_t *mbrSector) {
  return mbrSector[510] == 0x55 && mbrSector[511] == 0xaa;
}
