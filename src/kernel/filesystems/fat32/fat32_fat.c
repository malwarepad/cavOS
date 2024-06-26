#include <fat32.h>
#include <malloc.h>
#include <system.h>
#include <util.h>

uint32_t fat32FATcacheLookup(FAT32 *fat, uint32_t offset) {
  for (int i = 0; i < FAT32_CACHE_MAX; i++) {
    if (fat->cacheBase[i] == offset)
      return i;
  }

  return FAT32_CACHE_BAD;
}

void fat32FATcacheAdd(FAT32 *fat, uint32_t offset, uint8_t *bytes) {
  if (fat->cacheCurr >= FAT32_CACHE_MAX)
    fat->cacheCurr = 0;
  fat->cacheBase[fat->cacheCurr] = offset;
  memcpy(fat->cache[fat->cacheCurr], bytes,
         LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster));
  fat->cacheCurr++;
}

void fat32FATfetch(FAT32 *fat, uint32_t offsetSector, uint8_t *bytes) {
  uint32_t cacheRes = fat32FATcacheLookup(fat, offsetSector);
  if (cacheRes != FAT32_CACHE_BAD) {
    memcpy(bytes, fat->cache[cacheRes],
           LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster));
    return;
  }
  getDiskBytes(bytes, offsetSector, 1);
  fat32FATcacheAdd(fat, offsetSector, bytes);
}

uint32_t fat32FATtraverse(FAT32 *fat, uint32_t offset) {
  int bytesPerCluster = LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster);

  uint32_t offsetFAT = offset * 4; // every entry is sizeof(uint32_t) = 4
  uint32_t offsetSector = fat->offsetFats + (offsetFAT / bytesPerCluster);
  uint32_t offsetEntry = offsetFAT % bytesPerCluster;

  uint8_t *bytes = (uint8_t *)malloc(bytesPerCluster);
  fat32FATfetch(fat, offsetSector, bytes);

  uint32_t *retLocation = (uint32_t *)(&bytes[offsetEntry]);
  uint32_t  ret = (*retLocation) & 0x0FFFFFFF; // remember; we're on FAT32

  if (ret >= 0x0FFFFFF8) // end of cluster chain
    return 0;

  if (ret == 0x0FFFFFF7) // invalid/bad cluster
    return 0;

  return ret;
}
