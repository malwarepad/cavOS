#include <disk.h>
#include <fat32.h>
#include <linked_list.h>
#include <malloc.h>
#include <string.h>
#include <system.h>
#include <task.h>
#include <util.h>
#include <vfs.h>

// VFS special file management/creation
// Copyright (C) 2024 Panagiotis

OpenFile *fsRegisterNode(Task *task);
bool      fsUnregisterNode(Task *task, OpenFile *file);

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
  dummy->dir = 0;

  dummy->handlers = special->handlers;

  return dummy;
}

bool fsUserOpenSpecial(void **firstSpecial, char *filename, void *taskPtr,
                       int fd, VfsHandlers *specialHandlers) {
  Task *task = (Task *)taskPtr;

  spinlockCntWriteAcquire(&task->WLOCK_SPECIAL);
  SpecialFile *target = (SpecialFile *)LinkedListAllocate(
      (void **)(firstSpecial), sizeof(SpecialFile));
  spinlockCntWriteRelease(&task->WLOCK_SPECIAL);

  size_t filenameLen = strlength(filename) + 1; // null terminated
  void  *filenameBuff = malloc(filenameLen);
  memcpy(filenameBuff, filename, filenameLen);
  target->filename = filenameBuff;

  target->id = fd;
  target->handlers = specialHandlers;

  return true;
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
  spinlockCntWriteAcquire(&target->WLOCK_SPECIAL);
  bool ret = LinkedListRemove((void **)&target->firstSpecialFile, special);
  spinlockCntWriteRelease(&target->WLOCK_SPECIAL);
  return ret;
}

SpecialFile *fsUserSearchSpecialList(SpecialFile *firstSpecial, void *task,
                                     char *filename) {
  Task *target = (Task *)task;
  if (!target || !firstSpecial)
    return 0;
  spinlockCntReadAcquire(&target->WLOCK_SPECIAL);
  SpecialFile *browse = firstSpecial;
  while (browse) {
    size_t len1 = strlength(filename);
    size_t len2 = strlength(browse->filename);
    size_t fin = len1 > len2 ? len1 : len2;
    if (memcmp(browse->filename, filename, fin) == 0)
      break;
    browse = browse->next;
  }
  spinlockCntReadRelease(&target->WLOCK_SPECIAL);

  return browse;
}

SpecialFile *fsUserGetSpecialByFilename(void *task, char *filename) {
  SpecialFile *conventional =
      fsUserSearchSpecialList(currentTask->firstSpecialFile, task, filename);
  if (conventional)
    return conventional;

  SpecialFile *global =
      fsUserSearchSpecialList(firstGlobalSpecial, task, filename);
  return global; // can return 0
}

SpecialFile *fsUserGetSpecialById(void *taskPtr, int fd) {
  Task *task = (Task *)taskPtr;
  if (!task || !task->firstSpecialFile)
    return 0;
  spinlockCntReadAcquire(&task->WLOCK_SPECIAL);
  SpecialFile *browse = task->firstSpecialFile;
  while (browse) {
    if (browse->id == fd)
      break;
    browse = browse->next;
  }
  spinlockCntReadRelease(&task->WLOCK_SPECIAL);
  return browse;
}
