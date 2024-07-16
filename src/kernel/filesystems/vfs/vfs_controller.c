#include <disk.h>
#include <fat32.h>
#include <linked_list.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <util.h>
#include <vfs.h>

// Simple VFS abstraction to manage filesystems
// Copyright (C) 2024 Panagiotis

bool fsUnmount(MountPoint *mnt) {
  debugf("[vfs] Tried to unmount!\n");
  panic();
  LinkedListUnregister((void **)&firstMountPoint, mnt);

  // todo!
  // switch (mnt->filesystem) {
  // case FS_FATFS:
  //   f_unmount("/");
  //   break;
  // }

  free(mnt->prefix);
  free(mnt);

  return true;
}

bool isFat(mbr_partition *mbr) {
  uint8_t *rawArr = (uint8_t *)malloc(SECTOR_SIZE);
  getDiskBytes(rawArr, mbr->lba_first_sector, 1);

  bool ret = (rawArr[66] == 0x28 || rawArr[66] == 0x29);

  free(rawArr);

  return ret;
}

// prefix MUST end with '/': /mnt/handle/
MountPoint *fsMount(char *prefix, CONNECTOR connector, uint32_t disk,
                    uint8_t partition) {
  MountPoint *mount = (MountPoint *)LinkedListAllocate(
      (void **)&firstMountPoint, sizeof(MountPoint));

  uint32_t strlen = strlength(prefix);
  mount->prefix = (char *)(malloc(strlen + 1));
  memcpy(mount->prefix, prefix, strlen);
  mount->prefix[strlen] = '\0'; // null terminate

  mount->disk = disk;
  mount->partition = partition;
  mount->connector = connector;

  bool ret = false;
  switch (connector) {
  case CONNECTOR_AHCI:
    if (!openDisk(disk, partition, &mount->mbr)) {
      fsUnmount(mount);
      return 0;
    }

    if (isFat(&mount->mbr)) {
      mount->filesystem = FS_FATFS;
      ret = fat32Mount(mount);
    }
    break;
  default:
    debugf("[vfs] Tried to mount with bad connector! id{%d}\n", connector);
    ret = 0;
    break;
  }

  if (!ret) {
    fsUnmount(mount);
    return 0;
  }

  if (!systemDiskInit && strlength(prefix) == 1 && prefix[0] == '/')
    systemDiskInit = true;
  return mount;
}

MountPoint *fsDetermineMountPoint(char *filename) {
  MountPoint *largestAddr = 0;
  uint32_t    largestLen = 0;

  MountPoint *browse = firstMountPoint;
  while (browse) {
    if (strlength(browse->prefix) > largestLen &&
        memcmp(filename, browse->prefix, strlength(browse->prefix)) == 0) {
      largestAddr = browse;
      largestLen = strlength(browse->prefix);
    }
    browse = browse->next;
  }

  return largestAddr;
}

OpenFile *fsRegisterNode(Task *task) {
  return LinkedListAllocate((void **)&task->firstFile, sizeof(OpenFile));
}

bool fsUnregisterNode(Task *task, OpenFile *file) {
  SpecialFile *special = fsUserGetSpecialById(task, file->id);
  if (special)
    fsUserCloseSpecial(task, special);
  return LinkedListUnregister((void **)&task->firstFile, file);
}

// todo: save safeFilename too
OpenFile *fsUserSpecialDummyGen(void *task, int fd, SpecialFile *special,
                                int flags, int mode) {
  // we have a special file!
  OpenFile *dummy = fsRegisterNode((Task *)task);
  dummy->id = fd;

  dummy->flags = flags;
  dummy->mode = mode;
  dummy->pointer = 0;
  dummy->tmp1 = 0;

  dummy->mountPoint = (void *)MOUNT_POINT_SPECIAL;
  dummy->dir = special;

  return dummy;
}

bool fsCloseFsSpecific(OpenFile *file) {
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

bool fsOpenFsSpecific(char *filename, MountPoint *mnt, OpenFile *target) {
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

// TODO! flags! modes!
// todo: openId in a bitmap or smth, per task/kernel

char     *prefix = "/";
int       openId = 3;
OpenFile *fsOpenGeneric(char *filename, Task *task, int flags, int mode) {
  char *safeFilename = fsSanitize(task ? task->cwd : prefix, filename);
  // debugf("opening %s\n", safeFilename);

  SpecialFile *special = fsUserGetSpecialByFilename(task, safeFilename);
  if (special) {
    free(safeFilename);
    return fsUserSpecialDummyGen(task, openId++, special, flags, mode);
  }

  OpenFile *target = fsRegisterNode(task);
  target->id = openId++;
  target->mode = mode;
  target->flags = flags;

  target->pointer = 0;
  target->tmp1 = 0;

  MountPoint *mnt = fsDetermineMountPoint(safeFilename);
  if (!mnt) {
    // no mountpoint for this
    fsUnregisterNode(task, target);
    free(target);
    free(safeFilename);
    return 0;
  }
  target->mountPoint = mnt;

  bool res = fsOpenFsSpecific(safeFilename, mnt, target);
  free(safeFilename);

  if (!res) {
    // failed to open
    fsUnregisterNode(task, target);
    free(target);
    return 0;
  }

  return target;
}

bool fsUserOpenSpecial(char *filename, void *taskPtr, int fd,
                       SpecialHandlers *specialHandlers) {
  Task *task = (Task *)taskPtr;

  SpecialFile *target = (SpecialFile *)LinkedListAllocate(
      (void **)(&task->firstSpecialFile), sizeof(SpecialFile));

  size_t filenameLen = strlength(filename) + 1; // null terminated
  void  *filenameBuff = malloc(filenameLen);
  memcpy(filenameBuff, filename, filenameLen);
  target->filename = filenameBuff;

  target->id = fd;
  target->handlers = specialHandlers;

  return true;
}

// returns an ORPHAN!
OpenFile *fsUserDuplicateNodeUnsafe(OpenFile *original, SpecialFile *special) {
  OpenFile *orphan = (OpenFile *)malloc(sizeof(OpenFile));
  orphan->next = 0; // duh

  memcpy((void *)((size_t)orphan + sizeof(orphan->next)),
         (void *)((size_t)original + sizeof(original->next)),
         sizeof(OpenFile) - sizeof(orphan->next));

  if (orphan->dir) {
    if (orphan->mountPoint == MOUNT_POINT_SPECIAL) {
      if (special) {
        orphan->dir = special;
        return orphan;
      }

      panic();
      return 0;
    }
    switch (orphan->mountPoint->filesystem) {
    case FS_FATFS:
      orphan->dir = malloc(sizeof(FAT32OpenFd));
      memcpy(orphan->dir, original->dir, sizeof(FAT32OpenFd));
      break;
    default:
      debugf("[vfs] Tried to duplicateNode with bad filesystem! id{%d}\n",
             orphan->mountPoint->filesystem);
      return 0;
      break;
    }
  }

  return orphan;
}

OpenFile *fsUserDuplicateNode(void *taskPtr, OpenFile *original) {
  Task *task = (Task *)taskPtr;

  // special can be 0
  SpecialFile *special = 0;
  if (original->mountPoint == MOUNT_POINT_SPECIAL)
    special = original->dir;

  OpenFile *target = fsUserDuplicateNodeUnsafe(original, special);
  target->id = openId++;

  LinkedListPushFrontUnsafe((void **)(&task->firstFile), target);

  return target;
}

// returns an ORPHAN!
SpecialFile *fsUserDuplicateSpecialNodeUnsafe(SpecialFile *original) {
  SpecialFile *orphan = (SpecialFile *)malloc(sizeof(SpecialFile));
  orphan->next = 0; // duh

  memcpy((void *)((size_t)orphan + sizeof(orphan->next)),
         (void *)((size_t)original + sizeof(original->next)),
         sizeof(SpecialFile) - sizeof(orphan->next));

  size_t filenameLen = strlength(original->filename) + 1;
  orphan->filename = malloc(filenameLen);
  memcpy(orphan->filename, original->filename, filenameLen);

  return orphan;
}

bool fsUserCloseSpecial(void *task, SpecialFile *special) {
  Task *target = (Task *)task;
  return LinkedListRemove((void **)&target->firstSpecialFile, special);
}

SpecialFile *fsUserGetSpecialByFilename(void *task, char *filename) {
  Task *target = (Task *)task;
  if (!target || !target->firstSpecialFile)
    return 0;
  SpecialFile *browse = target->firstSpecialFile;
  while (browse) {
    size_t len1 = strlength(filename);
    size_t len2 = strlength(browse->filename);
    size_t fin = len1 > len2 ? len1 : len2;
    if (memcmp(browse->filename, filename, fin) == 0)
      break;
    browse = browse->next;
  }

  return browse;
}

SpecialFile *fsUserGetSpecialById(void *taskPtr, int fd) {
  Task *task = (Task *)taskPtr;
  if (!task || !task->firstSpecialFile)
    return 0;
  SpecialFile *browse = task->firstSpecialFile;
  while (browse) {
    if (browse->id == fd)
      break;
    browse = browse->next;
  }
  return browse;
}

OpenFile *fsUserGetNode(void *task, int fd) {
  Task     *target = (Task *)task;
  OpenFile *browse = target->firstFile;
  while (browse) {
    if (browse->id == fd)
      break;

    browse = browse->next;
  }

  if (!browse) {
    // might be a special file then
    SpecialFile *special = target->firstSpecialFile;

    while (special) {
      if (special->id == fd)
        break;

      special = special->next;
    }

    if (!special)
      return 0;

    return fsUserSpecialDummyGen(target, fd, special, O_RDWR, 0);
  }

  return browse;
}

OpenFile *fsKernelOpen(char *filename, int flags, uint32_t mode) {
  Task *target = taskGet(KERNEL_TASK_ID);
  return fsOpenGeneric(filename, target, flags, mode);
}

int fsUserOpen(void *task, char *filename, int flags, int mode) {
  // todo: modes & flags
  OpenFile *file = fsOpenGeneric(filename, (Task *)task, flags, mode);
  if (!file)
    return -ENOENT;

  return file->id;
}

bool fsCloseGeneric(OpenFile *file, Task *task) {
  fsUnregisterNode(task, file);

  bool res = fsCloseFsSpecific(file);

  free(file);
  return res;
}

bool fsKernelClose(OpenFile *file) {
  Task *target = taskGet(KERNEL_TASK_ID);
  return fsCloseGeneric(file, target);
}

int fsUserClose(void *task, int fd) {
  OpenFile *file = fsUserGetNode(task, fd);
  if (!file)
    return -EBADF;
  bool res = fsCloseGeneric(file, (Task *)task);
  if (res)
    return 0;
  else
    return -1;
}

uint32_t fsGetFilesize(OpenFile *file) {
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

uint32_t fsRead(OpenFile *file, uint8_t *out, uint32_t limit) {
  if (file->mountPoint == MOUNT_POINT_SPECIAL)
    return ((SpecialFile *)file->dir)->handlers->read(file, out, limit);

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

uint32_t fsWrite(OpenFile *file, uint8_t *in, uint32_t limit) {
  if (file->mountPoint == MOUNT_POINT_SPECIAL)
    return ((SpecialFile *)file->dir)->handlers->write(file, in, limit);

  if (1 == 1) // todo!
    return 0;

  uint32_t ret = 0;
  switch (file->mountPoint->filesystem) {
  case FS_FATFS: {
    // unsigned int write = 0;
    // bool         output = f_write(file->dir, in, limit, &write) == FR_OK;
    // if (output)
    //   ret = write;
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

bool fsWriteSync(OpenFile *file) {
  if (file->mountPoint == MOUNT_POINT_SPECIAL)
    return true;

  if (1 == 1) // todo!
    return false;

  bool ret = false;
  switch (file->mountPoint->filesystem) {
  case FS_FATFS:
    // if (f_sync(file->dir) == FR_OK)
    //   ret = true;
    break;
  default:
    debugf("[vfs] Tried to writeSync with bad filesystem! id{%d}\n",
           file->mountPoint->filesystem);
    ret = false;
    break;
  }
  return ret;
}

void fsReadFullFile(OpenFile *file, uint8_t *out) {
  switch (file->mountPoint->filesystem) {
  case FS_FATFS:
    fsRead(file, out, fsGetFilesize(file));
    break;
  default:
    debugf("[vfs] Tried to readFullFile with bad filesystem! id{%d}\n",
           file->mountPoint->filesystem);
    break;
  }
}

#define SEEK_SET 0  // start + offset
#define SEEK_CURR 1 // current + offset
#define SEEK_END 2  // end + offset
int fsUserSeek(void *task, uint32_t fd, int offset, int whence) {
  OpenFile *file = fsUserGetNode(task, fd);
  if (!file)
    return -1;
  int target = offset;
  if (whence == SEEK_SET)
    target += 0;
  else if (whence == SEEK_CURR)
    target += file->pointer;
  else if (whence == SEEK_END)
    target += fsGetFilesize(file);

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

int fsGetdents64(void *task, unsigned int fd, void *start,
                 unsigned int hardlimit) {
  // todo, special files, directories, etc
  OpenFile *file = fsUserGetNode(task, fd);
  if (!file)
    return -EBADF;
  return fat32Getdents64(file, start, hardlimit);
}
