#include <disk.h>
#include <fat32.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <util.h>

FAT32TraverseResult fat32Traverse(FAT32 *fat, uint32_t initDirectory,
                                  char *search, size_t searchLength) {
  uint8_t *bytes =
      (uint8_t *)malloc(LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster));
  uint32_t directory = initDirectory;

  // just in case mfs.fat didn't need to make an LFN
  int shortDot = fat32IsShortFilenamePossible(search, searchLength);

  uint8_t lfnName[LFN_MAX_TOTAL_CHARS] = {0};
  int     lfnLast = -1;

  while (true) {
    getDiskBytes(bytes, fat32ClusterToLBA(fat, directory),
                 fat->bootsec.sectors_per_cluster);

    for (int i = 0; i < LBA_TO_OFFSET(fat->bootsec.sectors_per_cluster);
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
        bool found = false;

        if (lfnLast >= 0) {
          int lfnLen = 0;
          while (lfnName[lfnLen++])
            ;
          lfnLen--; // without null terminator
          if (lfnLen < 0) {
            debugf("[fat32] Something is horribly wrong... lfnLen{%d}\n",
                   lfnLen);
            panic();
          }

          if (lfnLen == searchLength && memcmp(lfnName, search, lfnLen) == 0)
            found = true;

          lfnLast = -1;
        } else if (shortDot >= 0) {
          if (!memcmp(dir->name, search, shortDot ? shortDot : searchLength) &&
              (!shortDot || !memcmp(dir->ext, search + shortDot + 1,
                                    searchLength - shortDot - 1)))
            found = true;
        }
        memset(lfnName, 0, LFN_MAX_TOTAL_CHARS); // for good measure

        if (found) {
          FAT32TraverseResult ret;
          ret.directory = directory;
          ret.index = i;
          memcpy(&ret.dirEntry, dir, sizeof(FAT32DirectoryEntry));
          free(bytes);
          return ret;
        }
      }
    }

    directory = fat32FATtraverse(fat, directory);
    if (!directory)
      break;
  }

  FAT32TraverseResult ret = {0};
  return ret;
}

FAT32TraverseResult fat32TraversePath(FAT32 *fat, char *path,
                                      uint32_t directoryStarting) {
  uint32_t directory = directoryStarting;
  size_t   len = strlength(path);

  if (len == 1) { // meaning it's trying to open /
    FAT32TraverseResult res = {0};
    res.directory = -1;
    res.index = -1;
    res.dirEntry.attrib = FAT_ATTRIB_DIRECTORY;
    res.dirEntry.clusterhigh =
        SPLIT_32_HIGHER(fat->bootsec.extended_section.root_cluster);
    res.dirEntry.clusterlow =
        SPLIT_32_LOWER(fat->bootsec.extended_section.root_cluster);
    return res;
  }

  int lastslash = 0;
  for (int i = 1; i < len; i++) { // 1 to skip /[...]
    bool last = i == (len - 1);

    if (path[i] == '/' || last) {
      size_t length = i - lastslash - 1;
      if (last) // no need to remove trailing /
        length += 1;

      FAT32TraverseResult res =
          fat32Traverse(fat, directory, path + lastslash + 1, length);

      // return fail or last's success
      if (!res.directory || i == (len - 1))
        return res;

      directory =
          FAT_COMB_HIGH_LOW(res.dirEntry.clusterhigh, res.dirEntry.clusterlow);
      lastslash = i;
    }
  }

  // will never be reached but whatever
  FAT32TraverseResult res = {0};
  res.directory = 0;
  return res;
}
