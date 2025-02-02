#include <fat32.h>
#include <malloc.h>
#include <system.h>
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
    target->st_size = 0x1000;

    target->st_mode &= ~S_IFREG; // mark as dir
    target->st_mode |= S_IFDIR;
  } else
    target->st_size = res->dirEntry.filesize;

  target->st_blocks =
      (DivRoundUp(target->st_size, target->st_blksize) * target->st_blksize) /
      512;

  target->st_atime = fat32UnixTime(res->dirEntry.accessdate, 0);
  target->st_mtime =
      fat32UnixTime(res->dirEntry.modifieddate, res->dirEntry.modifiedtime);
  target->st_ctime =
      fat32UnixTime(res->dirEntry.createdate, res->dirEntry.createtime);
}

bool fat32Stat(MountPoint *mnt, char *filename, struct stat *target,
               char **symlinkResolve) {
  FAT32 *fat = FAT_PTR(mnt->fsInfo);

  FAT32TraverseResult res = fat32TraversePath(
      fat, filename, fat->bootsec.extended_section.root_cluster);
  if (!res.directory)
    return false;
  fat32StatInternal(&res, target);

  return true;
}

size_t fat32StatFd(OpenFile *fd, struct stat *target) {
  FAT32OpenFd *dir = FAT_DIR_PTR(fd->dir);

  FAT32TraverseResult res = {.directory = dir->directoryStarting,
                             .index = dir->index};
  memcpy(&res.dirEntry, &dir->dirEnt, sizeof(FAT32DirectoryEntry));
  fat32StatInternal(&res, target);
  return true;
}
