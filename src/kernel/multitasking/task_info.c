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
  } else
    spinlockRelease(&target->LOCK_PD);
}
