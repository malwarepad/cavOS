#include <ext2.h>
#include <fat32.h>
#include <linux.h>
#include <malloc.h>
#include <task.h>
#include <util.h>
#include <vfs.h>

// todo: special files & timestamps
bool fsStat(OpenFile *fd, stat *target) {
  if (!fd->handlers->stat) {
    debugf("[vfs] Lacks stat handler fd{%d}!\n", fd->id);
    panic();
  }
  return fd->handlers->stat(fd, target) == 0;
}

bool fsStatByFilename(void *task, char *filename, stat *target) {
  char *safeFilename = fsSanitize(((Task *)task)->cwd, filename);

  SpecialFile *special = fsUserGetSpecialByFilename(task, safeFilename);
  if (special) {
    free(safeFilename);
    return special->handlers->stat(0, target) == 0;
  }

  MountPoint *mnt = fsDetermineMountPoint(safeFilename);
  bool        ret = false;
  char       *strippedFilename = fsStripMountpoint(safeFilename, mnt);
  switch (mnt->filesystem) {
  case FS_FATFS:
    ret = fat32Stat(mnt->fsInfo, strippedFilename, target);
    break;
  case FS_EXT2:
    ret = ext2Stat(mnt->fsInfo, strippedFilename, target);
    break;
  default:
    debugf("[vfs] Tried to stat with bad filesystem! id{%d}\n",
           mnt->filesystem);
    break;
  }

  free(safeFilename);
  return ret;
}

bool fsLstatByFilename(void *task, char *filename, stat *target) {
  char *safeFilename = fsSanitize(((Task *)task)->cwd, filename);

  SpecialFile *special = fsUserGetSpecialByFilename(task, safeFilename);
  if (special) {
    free(safeFilename);
    return special->handlers->stat(0, target) == 0;
  }

  MountPoint *mnt = fsDetermineMountPoint(safeFilename);
  bool        ret = false;
  char       *strippedFilename = fsStripMountpoint(safeFilename, mnt);
  switch (mnt->filesystem) {
  case FS_FATFS:
    // fat32 doesn't support symbolic links anyways
    ret = fat32Stat(mnt->fsInfo, strippedFilename, target);
    break;
  case FS_EXT2:
    ret = ext2Lstat(mnt->fsInfo, strippedFilename, target);
    break;
  default:
    debugf("[vfs] Tried to stat with bad filesystem! id{%d}\n",
           mnt->filesystem);
    break;
  }

  free(safeFilename);
  return ret;
}
