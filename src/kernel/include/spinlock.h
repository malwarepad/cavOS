#include <stdatomic.h>

#include "types.h"

#ifndef SPINLOCK_H
#define SPINLOCK_H

// typedef struct SpinLock {
//   bool locked;
// } SpinLock;
// #define __SPINLOCK(name) static SpinLock name = {.locked = false}

typedef atomic_flag Spinlock;

void spinlockAcquire(Spinlock *lock);
void spinlockRelease(Spinlock *lock);

void spinlockWait(Spinlock *lock);

typedef struct SpinlockCnt {
  int64_t cnt;
} SpinlockCnt;

void spinlockCntReadAcquire(SpinlockCnt *lock);
void spinlockCntReadRelease(SpinlockCnt *lock);

void spinlockCntWriteAcquire(SpinlockCnt *lock);
void spinlockCntWriteRelease(SpinlockCnt *lock);

#endif
