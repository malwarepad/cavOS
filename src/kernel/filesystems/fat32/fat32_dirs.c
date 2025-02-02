#include <dents.h>
#include <fat32.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <util.h>

size_t fat32Getdents64(OpenFile *file, struct linux_dirent64 *start,
                       unsigned int hardlimit) {
  FAT32       *fat = FAT_PTR(file->mountPoint->fsInfo);
  FAT32OpenFd *fatDir = FAT_DIR_PTR(file->dir);

  if (!(fatDir->dirEnt.attrib & FAT_ATTRIB_DIRECTORY))
    return ERR(ENOTDIR);

  if (!fatDir->directoryCurr)
    return 0;

  size_t allocatedlimit = 0;

  int bytesPerCluster = LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster);

  uint8_t lfnName[LFN_MAX_TOTAL_CHARS] = {0};
  int     lfnLast = -1;

  uint8_t               *bytes = (uint8_t *)malloc(bytesPerCluster);
  struct linux_dirent64 *dirp = (struct linux_dirent64 *)start;

  while (true) {
    if (!fatDir->directoryCurr)
      goto cleanup;

    uint32_t offsetStarting = fatDir->ptr % bytesPerCluster;
    getDiskBytes(bytes, fat32ClusterToLBA(fat, fatDir->directoryCurr),
                 fat->bootsec.sectors_per_cluster);

    for (uint32_t i = offsetStarting; i < bytesPerCluster;
         i += sizeof(FAT32DirectoryEntry)) {
      FAT32DirectoryEntry *dir = (FAT32DirectoryEntry *)(&bytes[i]);
      FAT32LFN            *lfn = (FAT32LFN *)dir;

      if (dir->attrib == FAT_ATTRIB_LFN && !lfn->type) {
        int index = (lfn->order & ~LFN_ORDER_FINAL) - 1;

        if (index >= LFN_MAX) {
          debugf("[fat32] Invalid LFN index{%d} size! (>= 20)\n", index);
          panic();
        }

        if (lfn->order & LFN_ORDER_FINAL)
          lfnLast = index;

        fat32LFNmemcpy(lfnName, lfn, index);
      }

      if (dir->attrib == FAT_ATTRIB_DIRECTORY ||
          dir->attrib == FAT_ATTRIB_ARCHIVE) {
        // debugf("(%.8s)\n", dir->name);
        int lfnLen = 0;
        if (lfnLast >= 0) {
          while (lfnName[lfnLen++])
            ;
          lfnLen--; // without null terminator
          if (lfnLen < 0) {
            debugf("[fat32] Something is horribly wrong... lfnLen{%d}\n",
                   lfnLen);
            panic();
          }

          // we good!

          lfnLast = -1;
        } else {
          lfnLen = fat32SFNtoNormal(lfnName, dir);
        }

        unsigned char type = 0;
        if (dir->attrib & FAT_ATTRIB_DIRECTORY)
          type = CDT_DIR;
        else
          type = CDT_REG;

        DENTS_RES res = dentsAdd(
            start, &dirp, &allocatedlimit, hardlimit, (void *)lfnName, lfnLen,
            FAT_INODE_GEN(fatDir->directoryCurr, i / 32), type);

        if (res == DENTS_NO_SPACE) {
          allocatedlimit = ERR(EINVAL);
          goto cleanup;
        } else if (res == DENTS_RETURN)
          goto cleanup;

        memset(lfnName, 0, LFN_MAX_TOTAL_CHARS); // for good measure

        // traverse...
        // debugf("%ld\n", dirp->d_reclen);
      }
      fatDir->ptr += sizeof(FAT32DirectoryEntry);
    }

    fatDir->directoryCurr = fat32FATtraverse(fat, fatDir->directoryCurr);
    // debugf("moving on to %d\n", fatDir->directoryCurr);
    if (!fatDir->directoryCurr)
      break;
  }

cleanup:
  free(bytes);
  return allocatedlimit;
}
