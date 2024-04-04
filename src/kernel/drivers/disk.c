#include <ahci.h>
#include <disk.h>
#include <malloc.h>
#include <util.h>

// Multiple disk handler
// Copyright (C) 2022 Panagiotis

uint16_t mbr_partition_indexes[] = {MBR_PARTITION_1, MBR_PARTITION_2,
                                    MBR_PARTITION_3, MBR_PARTITION_4};

bool openDisk(uint32_t disk, uint8_t partition, mbr_partition *out) {
  disk = disk; // future plans

  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, 0x0, 1);
  // *out = *(mbr_partition *)(&rawArr[mbr_partition_indexes[partition]]);
  bool ret = validateMbr(rawArr);
  if (!ret)
    return false;
  memcpy(out, (void *)((size_t)rawArr + mbr_partition_indexes[partition]),
         sizeof(mbr_partition));
  free(rawArr);
  return true;
}

bool validateMbr(uint8_t *mbrSector) {
  return mbrSector[510] == 0x55 && mbrSector[511] == 0xaa;
}

void getDiskBytes(uint8_t *target_address, uint32_t LBA, uint8_t sector_count) {
  // todo: yeah, this STILL is NOT ideal

  PCI *browse = firstPCI;
  while (browse) {
    if (browse->driver == PCI_DRIVER_AHCI && ((ahci *)browse->extra)->sata)
      break;

    browse = browse->next;
  }

  if (!browse) {
    memset(target_address, 0, sector_count * SECTOR_SIZE);
    return;
  }

  ahci *target = (ahci *)browse->extra;
  int   pos = 0;
  while (!(target->sata & (1 << pos)))
    pos++;

  ahciRead(target, pos, &target->mem->ports[pos], LBA, 0, sector_count,
           target_address);
}
