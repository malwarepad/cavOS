#include <disk.h>
#include <ext2.h>
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

OpenFile *fsRegisterNode(Task *task) {
  spinlockCntWriteAcquire(&task->WLOCK_FILES);
  OpenFile *ret =
      LinkedListAllocate((void **)&task->firstFile, sizeof(OpenFile));
  spinlockCntWriteRelease(&task->WLOCK_FILES);
  return ret;
}

bool fsUnregisterNode(Task *task, OpenFile *file) {
  spinlockCntWriteAcquire(&task->WLOCK_FILES);
  bool ret = LinkedListUnregister((void **)&task->firstFile, file);
  spinlockCntWriteRelease(&task->WLOCK_FILES);
  return ret;
}

// TODO! flags! modes!
// todo: openId in a bitmap or smth, per task/kernel

char     *prefix = "/";
int       openId = 3;
OpenFile *fsOpenGeneric(char *filename, Task *task, int flags, int mode) {
  char *safeFilename = fsSanitize(task ? task->cwd : prefix, filename);

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
  target->handlers = mnt->handlers;

  if (flags & O_CLOEXEC)
    target->closeOnExec = true;

  char *strippedFilename = fsStripMountpoint(safeFilename, mnt);
  // check for open handler
  if (target->handlers->open) {
    char *symlink = 0;
    if (!target->handlers->open(strippedFilename, target, &symlink)) {
      // failed to open
      fsUnregisterNode(task, target);
      free(target);
      free(safeFilename);

      if (symlink) {
        // we came across a symbolic link
        char *symlinkResolved = fsResolveSymlink(mnt, symlink);
        free(symlink);
        OpenFile *res = fsOpenGeneric(symlinkResolved, task, flags, mode);
        free(symlinkResolved);
        return res;
      }
      return 0;
    }
    free(safeFilename);
  }
  return target;
}

// returns an ORPHAN!
OpenFile *fsUserDuplicateNodeUnsafe(OpenFile *original) {
  OpenFile *orphan = (OpenFile *)malloc(sizeof(OpenFile));
  orphan->next = 0; // duh

  memcpy((void *)((size_t)orphan + sizeof(orphan->next)),
         (void *)((size_t)original + sizeof(original->next)),
         sizeof(OpenFile) - sizeof(orphan->next));

  if (original->handlers->duplicate &&
      !original->handlers->duplicate(original, orphan)) {
    free(orphan);
    return 0;
  }

  return orphan;
}

OpenFile *fsUserDuplicateNode(void *taskPtr, OpenFile *original) {
  Task *task = (Task *)taskPtr;

  OpenFile *target = fsUserDuplicateNodeUnsafe(original);
  target->id = openId++;

  spinlockCntWriteAcquire(&task->WLOCK_FILES);
  LinkedListPushFrontUnsafe((void **)(&task->firstFile), target);
  spinlockCntWriteRelease(&task->WLOCK_FILES);

  return target;
}

OpenFile *fsUserGetNode(void *task, int fd) {
  Task *target = (Task *)task;
  spinlockCntReadAcquire(&target->WLOCK_FILES);
  OpenFile *browse = target->firstFile;
  while (browse) {
    if (browse->id == fd)
      break;

    browse = browse->next;
  }
  spinlockCntReadRelease(&target->WLOCK_FILES);

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

  bool res = file->handlers->close ? file->handlers->close(file) : true;
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

// todo: remove this bs
uint32_t fsGetFilesize(OpenFile *file) { return fsSpecificGetFilesize(file); }

uint32_t fsRead(OpenFile *file, uint8_t *out, uint32_t limit) {
  if (!file->handlers->read)
    return -EBADF;
  return file->handlers->read(file, out, limit);
}

uint32_t fsWrite(OpenFile *file, uint8_t *in, uint32_t limit) {
  if (!file->handlers->write)
    return -EBADF;
  return file->handlers->write(file, in, limit);
}

bool fsWriteSync(OpenFile *file) { return fsSpecificWriteSync(file); }

void fsReadFullFile(OpenFile *file, uint8_t *out) {
  fsRead(file, out, fsGetFilesize(file));
}

int fsUserSeek(void *task, uint32_t fd, int offset, int whence) {
  OpenFile *file = fsUserGetNode(task, fd);
  if (!file) // todo "special"
    return -1;
  int target = offset;
  if (whence == SEEK_SET)
    target += 0;
  else if (whence == SEEK_CURR)
    target += file->pointer;
  else if (whence == SEEK_END)
    target += fsGetFilesize(file);

  if (!file->handlers->seek)
    return -ESPIPE;

  return file->handlers->seek(file, target, offset, whence);
}

int fsReadlink(void *task, char *path, char *buf, int size) {
  Task       *target = task;
  char       *safeFilename = fsSanitize(target->cwd, path);
  MountPoint *mnt = fsDetermineMountPoint(safeFilename);
  int         ret = -1;

  char *symlink = 0;
  switch (mnt->filesystem) {
  case FS_FATFS:
    ret = -EINVAL;
    break;
  case FS_EXT2:
    ret =
        ext2Readlink((Ext2 *)(mnt->fsInfo), safeFilename, buf, size, &symlink);
    break;
  default:
    debugf("[vfs] Tried to readLink() with bad filesystem! id{%d}\n",
           mnt->filesystem);
    break;
  }

  free(safeFilename);

  if (symlink) {
    char *symlinkResolved = fsResolveSymlink(mnt, symlink);
    free(symlink);
    ret = fsReadlink(task, symlinkResolved, buf, size);
    free(symlinkResolved);
  }
  return ret;
}
