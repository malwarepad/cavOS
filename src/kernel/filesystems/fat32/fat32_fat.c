#include <fat32.h>
#include <malloc.h>
#include <system.h>
#include <util.h>

uint32_t fat32FATtraverse(FAT32 *fat, uint32_t offset) {
  uint32_t offsetFAT = offset * 4; // every entry is sizeof(uint32_t) = 4
  uint32_t offsetSector = fat->offsetFats + (offsetFAT / SECTOR_SIZE);
  uint32_t offsetEntry = offsetFAT % SECTOR_SIZE;

  uint8_t *bytes =
      (uint8_t *)malloc(LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster));
  getDiskBytes(bytes, offsetSector, 1);

  uint32_t *retLocation = (uint32_t *)(&bytes[offsetEntry]);
  uint32_t  ret = (*retLocation) & 0x0FFFFFFF; // remember; we're on FAT32

  if (ret >= 0x0FFFFFF8) // end of cluster chain
    return 0;

  if (ret == 0x0FFFFFF7) // invalid/bad cluster
    return 0;

  return ret;
}
