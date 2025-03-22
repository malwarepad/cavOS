#include <gdt.h>
#include <isr.h>
#include <kernel_helper.h>
#include <linked_list.h>
#include <linux.h>
#include <malloc.h>
#include <paging.h>
#include <pmm.h>
#include <schedule.h>
#include <stack.h>
#include <string.h>
#include <syscalls.h>
#include <task.h>
#include <util.h>
#include <vmm.h>

// Spliting struct Task into smaller chunks to allow sharing
// Copyright (C) 2025 Panagiotis

// CLONE_FS
TaskInfoFs *taskInfoFsAllocate() {
  TaskInfoFs *target = calloc(sizeof(TaskInfoFs), 1);
  target->utilizedBy = 1;
  target->cwd = calloc(2, 1);
  target->cwd[0] = '/';
  target->umask = S_IWGRP | S_IWOTH;
  return target;
}

void taskInfoFsDiscard(TaskInfoFs *target) {
  spinlockAcquire(&target->LOCK_FS);
  target->utilizedBy--;
  if (!target->utilizedBy) {
    free(target->cwd);
    free(target);
  } else
    spinlockRelease(&target->LOCK_FS);
}

TaskInfoFs *taskInfoFsClone(TaskInfoFs *old) {
  TaskInfoFs *new = taskInfoFsAllocate();

  spinlockAcquire(&old->LOCK_FS);
  new->umask = old->umask;
  size_t len = strlength(old->cwd) + 1;
  free(new->cwd); // no more default
  new->cwd = malloc(len);
  memcpy(new->cwd, old->cwd, len);
  spinlockRelease(&old->LOCK_FS);

  return new;
}

// CLONE_VM
TaskInfoPagedir *taskInfoPdAllocate(bool pagedir) {
  TaskInfoPagedir *target = calloc(sizeof(TaskInfoPagedir), 1);
  target->utilizedBy = 1;
  if (pagedir)
    target->pagedir = PageDirectoryAllocate();
  return target;
}

TaskInfoPagedir *taskInfoPdClone(TaskInfoPagedir *old) {
  TaskInfoPagedir *new = taskInfoPdAllocate(true);

  spinlockAcquire(&old->LOCK_PD);
  PageDirectoryUserDuplicate(old->pagedir, new->pagedir);
  spinlockRelease(&old->LOCK_PD);

  return new;
}

void taskInfoPdDiscard(TaskInfoPagedir *target) {
  spinlockAcquire(&target->LOCK_PD);
  target->utilizedBy--;
  if (!target->utilizedBy) {
    PageDirectoryFree(target->pagedir);
    // todo: find a safe way to free target
    // cannot be done w/the current layout as it's done inside taskKill and the
    // scheduler needs it in case it's switched in between (will point to
    // invalid/unsafe memory). maybe with overrides but we'll see later when the
    // system is more stable.
  } else
    spinlockRelease(&target->LOCK_PD);
}

// CLONE_FILES
TaskInfoFiles *taskInfoFilesAllocate() {
  TaskInfoFiles *target = calloc(sizeof(TaskInfoFiles), 1);
  target->utilizedBy = 1;
  return target;
}

// not needed since it's handled in vfs_controller.c
// TaskInfoFiles *taskInfoFilesClone(TaskInfoFiles *old);

void taskInfoFilesDiscard(TaskInfoFiles *target, void *task) {
  spinlockCntWriteAcquire(&target->WLOCK_FILES);
  target->utilizedBy--;
  if (!target->utilizedBy) {
    // we don't care about locks anymore (we are alone in the darkness)
    spinlockCntWriteRelease(&target->WLOCK_FILES);
    OpenFile *file = target->firstFile;
    while (file) {
      int id = file->id;
      file = file->next;
      fsUserClose(task, id);
    }
    free(target);
  } else
    spinlockCntWriteRelease(&target->WLOCK_FILES);
}
