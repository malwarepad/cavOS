#include "types.h"

#define SECTOR_SIZE 512

#ifndef DISK_H
#define DISK_H

#define MBR_PARTITION_1 0x01BE
#define MBR_PARTITION_2 0x01CE
#define MBR_PARTITION_3 0x01DE
#define MBR_PARTITION_4 0x01EE

#define MBR_BOOTABLE 0x80
#define MBR_REGULAR 0x00

typedef struct {
  uint8_t  status;
  uint8_t  chs_first_sector[3];
  uint8_t  type;
  uint8_t  chs_last_sector[3];
  uint32_t lba_first_sector;
  uint32_t sector_count;
} mbr_partition;

bool openDisk(uint32_t disk, uint8_t partition, mbr_partition *out);
bool validateMbr(uint8_t *mbrSector);
void getDiskBytes(uint8_t *target_address, uint32_t LBA, uint8_t sector_count);

#endif