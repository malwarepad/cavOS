#include <disk.h>
#include <ext2.h>
#include <fat32.h>
#include <malloc.h>
#include <poll.h>
#include <string.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <unixSocket.h>
#include <util.h>
#include <vfs.h>

// Simple VFS abstraction to manage filesystems
// Copyright (C) 2024 Panagiotis

OpenFile *fsRegisterNode(Task *task, size_t id) {
  TaskInfoFiles *files = task->infoFiles;
  spinlockCntWriteAcquire(&files->WLOCK_FILES);
  // debugf("reg %d\n", id);
  OpenFile *file = calloc(sizeof(OpenFile), 1);
  file->id = id;
  assert(AVLAllocate((void **)&files->firstFile, id, (avlval)file));
  spinlockCntWriteRelease(&files->WLOCK_FILES);
  return file;
}

bool fsUnregisterNode(Task *task, OpenFile *file) {
  TaskInfoFiles *files = task->infoFiles;
  spinlockCntWriteAcquire(&files->WLOCK_FILES);
  // debugf("unreg %d\n", file->id);
  bool ret = AVLUnregister((void **)&files->firstFile, file->id);
  spinlockCntWriteRelease(&files->WLOCK_FILES);
  return ret;
}

size_t fsIdFind(TaskInfoFiles *infoFiles) {
  size_t ret = -1;
  spinlockCntWriteAcquire(&infoFiles->WLOCK_FILES);
  for (size_t i = 0; i < infoFiles->rlimitFdsSoft; i++) {
    if (!bitmapGenericGet(infoFiles->fdBitmap, i)) {
      bitmapGenericSet(infoFiles->fdBitmap, i, true);
      ret = i;
      break;
    }
  }
  spinlockCntWriteRelease(&infoFiles->WLOCK_FILES);

  assert(ret != -1); // todo: RLIMIT errors
  return ret;
}

void fsIdRemove(TaskInfoFiles *infoFiles, size_t fd) {
  spinlockCntWriteAcquire(&infoFiles->WLOCK_FILES);
  bitmapGenericSet(infoFiles->fdBitmap, fd, false);
  spinlockCntWriteRelease(&infoFiles->WLOCK_FILES);
}

char     *prefix = "/";
OpenFile *fsOpenGeneric(char *filename, Task *task, int flags, int mode) {
  spinlockAcquire(&task->infoFs->LOCK_FS);
  char *safeFilename = fsSanitize(task ? task->infoFs->cwd : prefix, filename);
  spinlockRelease(&task->infoFs->LOCK_FS);

  size_t    id = fsIdFind(task->infoFiles);
  OpenFile *target = fsRegisterNode(task, id);
  // target->id = fsIdFind(task->infoFiles);
  target->mode = mode;
  target->flags = flags;

  target->pointer = 0;
  target->tmp1 = 0;

  MountPoint *mnt = fsDetermineMountPoint(safeFilename);
  if (!mnt) {
    // no mountpoint for this
    fsUnregisterNode(task, target);
    fsIdRemove(task->infoFiles, target->id);
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
    char  *symlink = 0;
    size_t ret =
        target->handlers->open(strippedFilename, flags, mode, target, &symlink);
    if (RET_IS_ERR(ret)) {
      // failed to open
      fsUnregisterNode(task, target);
      fsIdRemove(task->infoFiles, target->id);
      free(target);
      free(safeFilename);

      if (symlink && ret != ERR(ELOOP)) {
        // we came across a symbolic link
        char *symlinkResolved = fsResolveSymlink(mnt, symlink);
        free(symlink);
        OpenFile *res = fsOpenGeneric(symlinkResolved, task, flags, mode);
        free(symlinkResolved);
        return res;
      }
      return (OpenFile *)((size_t)(ret));
    }
    free(safeFilename);
  }
  return target;
}

bool fsUserDuplicateNodeUnsafe(OpenFile *original, OpenFile *orphan) {
  memcpy((void *)((size_t)orphan + sizeof(original->id)),
         (void *)((size_t)original + sizeof(original->id)),
         sizeof(OpenFile) - sizeof(original->id));

  return !original->handlers->duplicate ||
         original->handlers->duplicate(original, orphan);
}

// keep in mind that the taskPtr is of the target. original can be anywhere
OpenFile *fsUserDuplicateNode(void *taskPtr, OpenFile *original,
                              size_t suggid) {
  Task          *task = (Task *)taskPtr;
  TaskInfoFiles *files = task->infoFiles;

  size_t    id = suggid == (size_t)(-1) ? fsIdFind(files) : suggid;
  OpenFile *orphan = fsRegisterNode(task, id);
  // OpenFile *target = fsUserDuplicateNodeUnsafe(original);
  // target->id = fsIdFind(files);

  if (!fsUserDuplicateNodeUnsafe(original, orphan)) {
    free(orphan);
    return 0;
  }
  return orphan;
}

OpenFile *fsUserGetNode(void *task, int fd) {
  Task          *target = (Task *)task;
  TaskInfoFiles *files = target->infoFiles;
  spinlockCntReadAcquire(&files->WLOCK_FILES);
  OpenFile *browse = (OpenFile *)AVLLookup(files->firstFile, fd);
  spinlockCntReadRelease(&files->WLOCK_FILES);

  return browse;
}

OpenFile *fsKernelOpen(char *filename, int flags, uint32_t mode) {
  Task     *target = taskGet(KERNEL_TASK_ID);
  OpenFile *ret = fsOpenGeneric(filename, target, flags, mode);
  if (RET_IS_ERR((size_t)(ret)))
    return 0;
  return ret;
}

size_t fsUserOpen(void *task, char *filename, int flags, int mode) {
  if (flags & FASYNC) {
    debugf("[syscalls::fs] FATAL! Tried to open %s with O_ASYNC!\n", filename);
    return ERR(ENOSYS);
  }
  OpenFile *file = fsOpenGeneric(filename, (Task *)task, flags, mode);
  if (RET_IS_ERR((size_t)file))
    return (size_t)file;

  return file->id;
}

bool fsCloseGeneric(OpenFile *file, Task *task) {
  spinlockAcquire(&file->LOCK_OPERATIONS); // once and never again haha
  fsUnregisterNode(task, file);

  bool res = file->handlers->close ? file->handlers->close(file) : true;
  epollCloseNotify(file);
  if (!(file->closeFlags & VFS_CLOSE_FLAG_RETAIN_ID))
    fsIdRemove(task->infoFiles, file->id);
  free(file);
  return res;
}

bool fsKernelClose(OpenFile *file) {
  Task *target = taskGet(KERNEL_TASK_ID);
  return fsCloseGeneric(file, target);
}

size_t fsUserClose(void *task, int fd) {
  OpenFile *file = fsUserGetNode(task, fd);
  if (!file)
    return ERR(EBADF);
  bool res = fsCloseGeneric(file, (Task *)task);
  if (res)
    return 0;
  else
    return -1;
}

size_t fsGetFilesize(OpenFile *file) {
  spinlockAcquire(&file->LOCK_OPERATIONS);
  size_t ret = file->handlers->getFilesize(file);
  spinlockRelease(&file->LOCK_OPERATIONS);
  return ret;
}

size_t fsRead(OpenFile *file, uint8_t *out, uint32_t limit) {
  size_t ret = -1;
  spinlockAcquire(&file->LOCK_OPERATIONS);
  if (!file->handlers->read) {
    if (file->handlers->recvfrom) { // we got a socket!
      ret = file->handlers->recvfrom(file, out, limit, 0, 0, 0);
      goto cleanup;
    }
    ret = ERR(EBADF);
    goto cleanup;
  }
  ret = file->handlers->read(file, out, limit);
cleanup:
  spinlockRelease(&file->LOCK_OPERATIONS);
  return ret;
}

size_t fsWrite(OpenFile *file, uint8_t *in, uint32_t limit) {
  size_t ret = -1;
  spinlockAcquire(&file->LOCK_OPERATIONS);
  if (!(file->flags & O_RDWR) && !(file->flags & O_WRONLY)) {
    ret = ERR(EBADF);
    goto cleanup;
  }
  if (!file->handlers->write) {
    if (file->handlers->sendto) { // we got a socket!
      ret = file->handlers->sendto(file, in, limit, 0, 0, 0);
      goto cleanup;
    }
    ret = ERR(EBADF);
    goto cleanup;
  }
  ret = file->handlers->write(file, in, limit);
cleanup:
  spinlockRelease(&file->LOCK_OPERATIONS);
  return ret;
}

size_t fsUserSeek(void *task, uint32_t fd, int offset, int whence) {
  OpenFile *file = fsUserGetNode(task, fd);
  if (!file) // todo "special"
    return -1;

  spinlockAcquire(&file->LOCK_OPERATIONS);
  int target = offset;
  if (whence == SEEK_SET)
    target += 0;
  else if (whence == SEEK_CURR)
    target += file->pointer;
  else if (whence == SEEK_END)
    target += file->handlers->getFilesize(file);

  size_t ret = 0;
  if (!file->handlers->seek) {
    ret = ERR(ESPIPE);
    goto cleanup;
  }

  ret = file->handlers->seek(file, target, offset, whence);
cleanup:
  spinlockRelease(&file->LOCK_OPERATIONS);
  return ret;
}

size_t fsReadlink(void *task, char *path, char *buf, int size) {
  Task *target = task;
  spinlockAcquire(&target->infoFs->LOCK_FS);
  char *safeFilename = fsSanitize(target->infoFs->cwd, path);
  spinlockRelease(&target->infoFs->LOCK_FS);
  MountPoint *mnt = fsDetermineMountPoint(safeFilename);
  size_t      ret = -1;

  char *symlink = 0;
  if (!mnt->readlink) {
    free(safeFilename);
    return ERR(EINVAL);
  }
  char *strippedFilename = fsStripMountpoint(safeFilename, mnt);
  ret = mnt->readlink(mnt, strippedFilename, buf, size, &symlink);

  free(safeFilename);

  if (symlink) {
    char *symlinkResolved = fsResolveSymlink(mnt, symlink);
    free(symlink);
    ret = fsReadlink(task, symlinkResolved, buf, size);
    free(symlinkResolved);
  }
  return ret;
}

size_t fsMkdir(void *task, char *path, uint32_t mode) {
  Task *target = (Task *)task;
  spinlockAcquire(&target->infoFs->LOCK_FS);
  char *safeFilename = fsSanitize(target->infoFs->cwd, path);
  spinlockRelease(&target->infoFs->LOCK_FS);
  MountPoint *mnt = fsDetermineMountPoint(safeFilename);

  size_t ret = 0;

  char *symlink = 0;
  if (mnt->mkdir) {
    ret = mnt->mkdir(mnt, safeFilename, mode, &symlink);
  } else {
    ret = ERR(EROFS);
  }

  free(safeFilename);

  if (symlink) {
    char *symlinkResolved = fsResolveSymlink(mnt, symlink);
    free(symlink);
    ret = fsMkdir(task, symlinkResolved, mode);
    free(symlinkResolved);
  }

  return ret;
}

size_t fsUnlink(void *task, char *path, bool directory) {
  Task *target = (Task *)task;
  spinlockAcquire(&target->infoFs->LOCK_FS);
  char *safeFilename = fsSanitize(target->infoFs->cwd, path);
  spinlockRelease(&target->infoFs->LOCK_FS);
  MountPoint *mnt = fsDetermineMountPoint(safeFilename);

  size_t ret = 0;
  if (unixSocketUnlinkNotify(safeFilename)) {
    free(safeFilename);
    return 0;
  }

  char *symlink = 0;
  if (mnt->delete) {
    ret = mnt->delete(mnt, safeFilename, directory, &symlink);
  } else {
    ret = ERR(EROFS);
  }

  free(safeFilename);

  if (symlink) {
    char *symlinkResolved = fsResolveSymlink(mnt, symlink);
    free(symlink);
    ret = fsUnlink(task, symlinkResolved, directory);
    free(symlinkResolved);
  }

  return ret;
}

size_t fsLink(void *task, char *oldpath, char *newpath) {
  Task *target = (Task *)task;
  spinlockAcquire(&target->infoFs->LOCK_FS);
  char *oldpathSafe = fsSanitize(target->infoFs->cwd, oldpath);
  char *newpathSafe = fsSanitize(target->infoFs->cwd, newpath);
  spinlockRelease(&target->infoFs->LOCK_FS);
  MountPoint *mnt = fsDetermineMountPoint(oldpathSafe);
  if (fsDetermineMountPoint(newpathSafe) != mnt) {
    free(oldpathSafe);
    free(newpathSafe);
    return ERR(EXDEV);
  }

  size_t ret = 0;

  char *symlinkold = 0;
  char *symlinknew = 0;
  if (mnt->delete) {
    ret = mnt->link(mnt, oldpathSafe, newpathSafe, &symlinkold, &symlinknew);
  } else {
    ret = ERR(EPERM);
  }

  free(oldpathSafe);
  free(newpathSafe);

  char *old = oldpath;
  char *new = newpath;
  if (symlinkold)
    old = fsResolveSymlink(mnt, symlinkold);
  if (symlinknew)
    new = fsResolveSymlink(mnt, symlinknew);

  if (symlinkold)
    free(symlinkold);
  if (symlinknew)
    free(symlinknew);

  if (symlinkold || symlinknew)
    ret = fsLink(task, old, new);

  if (symlinkold)
    free(old);
  if (symlinknew)
    free(new);

  return ret;
}

// shared for fake filesystems etc
size_t fsSimpleSeek(OpenFile *file, size_t target, long int offset,
                    int whence) {
  // we're using the official ->pointer so no need to worry about much
  file->pointer = target;
  return 0;
}
