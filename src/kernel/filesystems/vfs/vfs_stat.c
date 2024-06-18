#include <linux.h>
#include <malloc.h>
#include <util.h>
#include <vfs.h>

#include "../fatfs/ff.h"

// todo: special files & timestamps
bool fsStatGeneric(char *filename, OpenFile *fd, stat *target) {
  bool ret = false;

  target->st_dev = 69;     // haha
  target->st_ino = rand(); // todo!
  target->st_mode = S_IFREG | S_IRUSR | S_IWUSR;
  target->st_nlink = 1;
  target->st_uid = 0;
  target->st_gid = 0;
  target->st_rdev = 0;
  target->st_blksize = 0x1000;

  if (fd) {
    switch (fd->mountPoint->filesystem) {
    case FS_FATFS: {
      ret = true;

      if (ret) {
        target->st_size = f_size((FIL *)(fd->dir)); // todo: directories
        target->st_blocks = DivRoundUp(target->st_size, 512);
        target->st_atime = 69;
        target->st_mtime = 69;
        target->st_ctime = 69;
      }

      break;
    }
    default:
      debugf("[vfs] Tried to stat with bad filesystem! id{%d}\n",
             fd->mountPoint->filesystem);
      ret = false;
      break;
    }
  } else if (filename) {
    char *safeFilename = fsSanitize(filename);

    if (safeFilename[0] == '/' && safeFilename[1] == '\0') {
      target->st_size = 0;
      target->st_blocks = (DivRoundUp(target->st_size, target->st_blksize) *
                           target->st_blksize) /
                          512;
      target->st_atime = 69;
      target->st_mtime = 69;
      target->st_ctime = 69;

      target->st_mode &= ~S_IFREG;
      target->st_mode |= S_IFDIR;
      return true;
    }

    MountPoint *mnt = fsDetermineMountPoint(safeFilename);
    if (!mnt) {
      free(safeFilename);
      return false;
    }

    switch (mnt->filesystem) {
    case FS_FATFS: {
      FILINFO filinfo = {0};

      if (f_stat(safeFilename, &filinfo) == FR_OK)
        ret = true;
      free(safeFilename);

      if (ret) {
        target->st_size = filinfo.fsize;
        target->st_blocks = (DivRoundUp(target->st_size, target->st_blksize) *
                             target->st_blksize) /
                            512;
        target->st_atime = 69;
        target->st_mtime = 69;
        target->st_ctime = 69;

        if (filinfo.fattrib & AM_DIR) {
          target->st_mode &= ~S_IFREG;
          target->st_mode |= S_IFDIR;
        }
      }

      break;
    }
    default:
      debugf("[vfs] Tried to stat with bad filesystem! id{%d}\n",
             mnt->filesystem);
      ret = false;
      break;
    }
  }
  return ret;
}

bool fsStat(OpenFile *fd, stat *target) { return fsStatGeneric(0, fd, target); }

bool fsStatByFilename(char *filename, stat *target) {
  return fsStatGeneric(filename, 0, target);
}
