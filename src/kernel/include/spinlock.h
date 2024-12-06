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

typedef struct SpinlockCnt {
  Spinlock LOCK; // todo!
  int64_t  cnt;
} SpinlockCnt;

typedef struct Semaphore {
  Spinlock LOCK;
  uint32_t cnt;
  uint8_t  invalid;
} Semaphore;

void spinlockCntReadAcquire(SpinlockCnt *lock);
void spinlockCntReadRelease(SpinlockCnt *lock);

void spinlockCntWriteAcquire(SpinlockCnt *lock);
void spinlockCntWriteRelease(SpinlockCnt *lock);

bool semaphoreWait(Semaphore *sem, uint32_t timeout);
void semaphorePost(Semaphore *sem);

#endif
