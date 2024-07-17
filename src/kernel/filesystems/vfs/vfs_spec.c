#include <disk.h>
#include <fat32.h>
#include <linked_list.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <util.h>
#include <vfs.h>

// VFS subsystem to manage real files
// "am I even real?" - mp 2024
// Copyright (C) 2024 Panagiotis

bool fsSpecificClose(OpenFile *file) {
  if (file->mountPoint == MOUNT_POINT_SPECIAL)
    return true;

  bool res = false;
  switch (file->mountPoint->filesystem) {
  case FS_FATFS:
    res = fat32Close(file->mountPoint, file);
    break;
  default:
    debugf("[vfs] Tried to close with bad filesystem! id{%d}\n",
           file->mountPoint->filesystem);
    res = false;
    break;
  }

  return res;
}

bool fsSpecificOpen(char *filename, MountPoint *mnt, OpenFile *target) {
  bool res = false;
  /*char *strippedFilename = (char *)((size_t)filename + strlength(mnt->prefix)
     - 1); // -1 for putting start slash*/
  switch (mnt->filesystem) {
  case FS_FATFS:
    res = fat32Open(mnt, target, filename);
    break;
  default:
    debugf("[vfs] Tried to open with bad filesystem! id{%d}\n",
           target->mountPoint->filesystem);
    res = false;
    break;
  }
  return res;
}

int fsSpecificRead(OpenFile *file, uint8_t *out, size_t limit) {
  uint32_t ret = 0;
  switch (file->mountPoint->filesystem) {
  case FS_FATFS: {
    ret = fat32Read(file->mountPoint, file, out, limit);
    break;
  }
  default:
    debugf("[vfs] Tried to read with bad filesystem! id{%d}\n",
           file->mountPoint->filesystem);
    ret = 0;
    break;
  }
  return ret;
}

int fsSpecificWrite(OpenFile *file, uint8_t *in, size_t limit) {
  if (1 == 1) // todo!
    return 0;

  uint32_t ret = 0;
  switch (file->mountPoint->filesystem) {
  case FS_FATFS: {
    // a
    break;
  }
  default:
    debugf("[vfs] Tried to write with bad filesystem! id{%d}\n",
           file->mountPoint->filesystem);
    ret = 0;
    break;
  }
  return ret;
}

bool fsSpecificWriteSync(OpenFile *file) {
  if (1 == 1) // todo!
    return false;

  bool ret = false;
  switch (file->mountPoint->filesystem) {
  case FS_FATFS:
    // a
    break;
  default:
    debugf("[vfs] Tried to writeSync with bad filesystem! id{%d}\n",
           file->mountPoint->filesystem);
    ret = false;
    break;
  }
  return ret;
}

size_t fsSpecificGetFilesize(OpenFile *file) {
  switch (file->mountPoint->filesystem) {
  case FS_FATFS:
    return fat32GetFilesize(file);
    break;
  default:
    debugf("[vfs] Tried to getFilesize with bad filesystem! id{%d}\n",
           file->mountPoint->filesystem);
    return 0;
    break;
  }

  return 0;
}

int fsSpecificSeek(OpenFile *file, int target, int offset, int whence) {
  bool ret = false;
  switch (file->mountPoint->filesystem) {
  case FS_FATFS:
    // "hack" because openfile ptr is not used
    if (whence == SEEK_CURR)
      target += ((FAT32OpenFd *)file->dir)->ptr;
    // debugf("moving to %d\n", target);
    ret = fat32Seek(file->mountPoint, file, target);
    break;
  default:
    debugf("[vfs] Tried to seek with bad filesystem! id{%d}\n",
           file->mountPoint->filesystem);
    ret = false;
    break;
  }
  if (!ret)
    return -1;
  return target;
}

// returns an ORPHAN!
bool fsSpecificDuplicateNodeUnsafe(OpenFile *original, OpenFile *orphan) {
  switch (orphan->mountPoint->filesystem) {
  case FS_FATFS:
    orphan->dir = malloc(sizeof(FAT32OpenFd));
    memcpy(orphan->dir, original->dir, sizeof(FAT32OpenFd));

    size_t len = strlength(original->dirname) + 1;
    orphan->dirname = (char *)malloc(len);
    memcpy(orphan->dirname, original->dirname, len);
    break;
  default:
    debugf("[vfs] Tried to duplicateNode with bad filesystem! id{%d}\n",
           orphan->mountPoint->filesystem);
    return 0;
    break;
  }
  return true;
}

int fsSpecificStat(OpenFile *fd, stat *target) {
  bool ret = false;
  switch (fd->mountPoint->filesystem) {
  case FS_FATFS: {
    ret = fat32StatFd(fd->mountPoint->fsInfo, fd, target);
    break;
  }
  default:
    debugf("[vfs] Tried to stat with bad filesystem! id{%d}\n",
           fd->mountPoint->filesystem);
    break;
  }

  return ret ? 0 : -1;
}

int fbSpecificIoctl(OpenFile *fd, uint64_t request, void *arg) {
  return -ENOTTY;
}

// todo: no 0s
SpecialHandlers fsSpecific = {.duplicate = fsSpecificDuplicateNodeUnsafe,
                              .ioctl = fbSpecificIoctl,
                              .mmap = 0,
                              .read = fsSpecificRead,
                              .stat = fsSpecificStat,
                              .write = fsSpecificWrite};
