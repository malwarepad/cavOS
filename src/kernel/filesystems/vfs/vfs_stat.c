#include <fat32.h>
#include <linux.h>
#include <malloc.h>
#include <task.h>
#include <util.h>
#include <vfs.h>

// todo: special files & timestamps
bool fsStat(OpenFile *fd, stat *target) {
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
  switch (mnt->filesystem) {
  case FS_FATFS: {
    ret = fat32Stat(mnt->fsInfo, safeFilename, target);
    break;
  }
  default:
    debugf("[vfs] Tried to stat with bad filesystem! id{%d}\n",
           mnt->filesystem);
    break;
  }

  free(safeFilename);
  return ret;
}
