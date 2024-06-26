#include <disk.h>
#include <fat32.h>
#include <malloc.h>
#include <system.h>
#include <timer.h>
#include <util.h>

// todo: directories are still rough around the edges...
// also, make the vfs remove mountpoint prefixes before handing them off to here
// also, optimization is REALLY needed!

bool fat32Mount(MountPoint *mount) {
  // assign fsInfo
  mount->fsInfo = malloc(sizeof(FAT32));
  memset(mount->fsInfo, 0, sizeof(FAT32));
  FAT32 *fat = FAT_PTR(mount->fsInfo);

  // base offset
  fat->offsetBase = mount->mbr.lba_first_sector; // 2048 (in LBA)

  // get first sector
  uint8_t firstSec[SECTOR_SIZE] = {0};
  getDiskBytes(firstSec, fat->offsetBase, 1);

  // store it
  memcpy(&fat->bootsec, firstSec, sizeof(FAT32BootSector));

  if (fat->bootsec.bytes_per_sector != SECTOR_SIZE) {
    debugf("[fat32] What kind of devil-made FAT partition doesn't have 512 "
           "bytes / sector?\n");
    panic();
  }

  // setup some other offsets so it's easier later
  fat->offsetFats = fat->offsetBase + fat->bootsec.reserved_sector_count;
  fat->offsetClusters =
      fat->offsetFats +
      fat->bootsec.table_count * fat->bootsec.extended_section.table_size_32;

  for (int i = 0; i < FAT32_CACHE_MAX; i++) {
    fat->cacheBase[i] = FAT32_CACHE_BAD;
    fat->cache[i] = malloc(LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster));
    memset(fat->cache[i], 0, LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster));
  }

  // done :")
  return true;
}

bool fat32Open(MountPoint *mount, OpenFile *fd, char *filename) {
  FAT32 *fat = FAT_PTR(mount->fsInfo);

  // first make sure it.. exists!
  FAT32TraverseResult res = fat32TraversePath(fat, filename);
  if (!res.directory)
    return false;

  // allocate some space
  fd->dir = malloc(sizeof(FAT32OpenFd));
  FAT32OpenFd *dir = FAT_DIR_PTR(fd->dir);
  memset(dir, 0, sizeof(FAT32OpenFd));

  // fill in some initial info
  dir->ptr = 0;
  dir->index = res.index;
  dir->directoryStarting = res.directory;
  dir->directoryCurr =
      FAT_COMB_HIGH_LOW(res.dirEntry.clusterhigh, res.dirEntry.clusterlow);
  memcpy(&dir->dirEnt, &res.dirEntry, sizeof(FAT32DirectoryEntry));

  return true;
}

int fat32Read(MountPoint *mount, OpenFile *fd, uint8_t *buff, int limit) {
  FAT32       *fat = FAT_PTR(mount->fsInfo);
  FAT32OpenFd *dir = FAT_DIR_PTR(fd->dir);

  if (dir->dirEnt.attrib & FAT_ATTRIB_DIRECTORY)
    return 0;

  int curr = 0; // will be used to return
  int bytesPerCluster = LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster);
  // ^ is used everywhere!

  uint8_t *bytes = (uint8_t *)malloc(bytesPerCluster);

  while (true) {
    int offsetStarting = dir->ptr % bytesPerCluster; // remainder
    getDiskBytes(bytes, fat32ClusterToLBA(fat, dir->directoryCurr),
                 fat->bootsec.sectors_per_cluster);

    for (int i = offsetStarting; i < bytesPerCluster; i++) {
      if (curr >= limit || dir->ptr >= dir->dirEnt.filesize) {
        free(bytes);
        return curr;
      }

      if (buff)
        buff[curr] = bytes[i];

      dir->ptr++;
      curr++;
    }

    dir->directoryCurr = fat32FATtraverse(fat, dir->directoryCurr);
  }

  free(bytes);
  return curr;
}

bool fat32Seek(MountPoint *mount, OpenFile *fd, uint32_t target) {
  FAT32       *fat = FAT_PTR(mount->fsInfo);
  FAT32OpenFd *dir = FAT_DIR_PTR(fd->dir);

  if (dir->dirEnt.attrib & FAT_ATTRIB_DIRECTORY)
    return 0;

  if (target > dir->dirEnt.filesize)
    return false;

  dir->ptr = target;
  dir->directoryCurr =
      FAT_COMB_HIGH_LOW(dir->dirEnt.clusterhigh, dir->dirEnt.clusterlow);

  // this REALLY needs optimization!
  int clustersToBeSkipped =
      target / LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster);
  for (int i = 0; i < clustersToBeSkipped; i++) {
    dir->directoryCurr = fat32FATtraverse(fat, dir->directoryCurr);
    if (!dir->directoryCurr)
      return false;
  }

  return true;
}

uint32_t fat32GetFilesize(OpenFile *fd) {
  FAT32OpenFd *dir = FAT_DIR_PTR(fd->dir);

  if (dir->dirEnt.attrib & FAT_ATTRIB_DIRECTORY)
    return 0;

  return dir->dirEnt.filesize;
}

bool fat32Close(MountPoint *mount, OpenFile *fd) {
  // :p
  free(fd->dir);
  return true;
}
