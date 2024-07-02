#include <fat32.h>
#include <malloc.h>
#include <util.h>

void fat32StatInternal(FAT32TraverseResult *res, struct stat *target) {
  target->st_dev = 69;                                        // haha
  target->st_ino = FAT_INODE_GEN(res->directory, res->index); // could work
  target->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IXUSR;
  target->st_nlink = 1;
  target->st_uid = 0;
  target->st_gid = 0;
  target->st_rdev = 0;
  target->st_blksize = 0x1000;

  if (res->dirEntry.attrib & FAT_ATTRIB_DIRECTORY) {
    target->st_size = 0;

    target->st_mode &= ~S_IFREG; // mark as dir
    target->st_mode |= S_IFDIR;
  } else
    target->st_size = res->dirEntry.filesize;

  target->st_blocks =
      (DivRoundUp(target->st_size, target->st_blksize) * target->st_blksize) /
      512;

  // todo!
  target->st_atime = 69;
  target->st_mtime = 69;
  target->st_ctime = 69;
}

bool fat32Stat(FAT32 *fat, char *filename, struct stat *target) {
  FAT32TraverseResult res = fat32TraversePath(fat, filename);
  if (!res.directory)
    return false;
  fat32StatInternal(&res, target);

  return true;
}

bool fat32StatFd(FAT32 *fat, OpenFile *fd, struct stat *target) {
  FAT32OpenFd *dir = FAT_DIR_PTR(fd->dir);

  FAT32TraverseResult res = {.directory = dir->directoryStarting,
                             .index = dir->index};
  memcpy(&res.dirEntry, &dir->dirEnt, sizeof(FAT32DirectoryEntry));
  fat32StatInternal(&res, target);
  return true;
}
