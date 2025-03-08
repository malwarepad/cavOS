#include "task.h"
#include "types.h"

#ifndef KERNEL_HELPER_H
#define KERNEL_HELPER_H

Task *netHelperTask;
void  kernelHelpEntry();

Spinlock LOCK_REAPER;
Task    *reaperTask;

#define MAX_SPINLOCK_QUEUE 25
Spinlock LOCK_SPINLOCK_QUEUE;
typedef struct SpinlockHelperQueue {
  bool      valid;
  Spinlock *target;
  Task     *task; // check for state
} SpinlockHelperQueue;
SpinlockHelperQueue spinlockHelperQueue[MAX_SPINLOCK_QUEUE];

void initiateKernelThreads();

#endif