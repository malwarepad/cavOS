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

  MountPoint *mnt = fsDetermineMountPoint(safeFilename);
  bool        ret = false;
  char       *strippedFilename = fsStripMountpoint(safeFilename, mnt);

  if (!mnt->stat)
    goto cleanup;

  ret = mnt->stat(mnt, strippedFilename, target);

cleanup:
  free(safeFilename);
  return ret;
}

bool fsLstatByFilename(void *task, char *filename, stat *target) {
  char *safeFilename = fsSanitize(((Task *)task)->cwd, filename);

  MountPoint *mnt = fsDetermineMountPoint(safeFilename);
  bool        ret = false;
  char       *strippedFilename = fsStripMountpoint(safeFilename, mnt);

  if (!mnt->lstat)
    goto cleanup;

  ret = mnt->lstat(mnt, strippedFilename, target);

cleanup:
  free(safeFilename);
  return ret;
}
