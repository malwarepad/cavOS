#include <spinlock.h>
#include <system.h>
#include <timer.h>

// Various thread-safe locking mechanisms
// Copyright (C) 2024 Panagiotis

void spinlockAcquire(Spinlock *lock) {
  while (atomic_flag_test_and_set_explicit(lock, memory_order_acquire))
    handControl();
}

void spinlockRelease(Spinlock *lock) {
  atomic_flag_clear_explicit(lock, memory_order_release);
}

// Cnt spinlock is basically just a counter that increases for every read
// operation. When something has to modify, it waits for it to become 0 and
// makes it -1, not permitting any reads. Useful for linked lists..

void spinlockCntReadAcquire(SpinlockCnt *lock) {
  while (true) {
    spinlockAcquire(&lock->LOCK);
    if (lock->cnt > -1) {
      lock->cnt++;
      goto cleanup;
    }
    spinlockRelease(&lock->LOCK);
    handControl();
  }

cleanup:
  spinlockRelease(&lock->LOCK);
}

void spinlockCntReadRelease(SpinlockCnt *lock) {
  spinlockAcquire(&lock->LOCK);
  if (lock->cnt < 0) {
    debugf("[spinlock] Something very bad is going on...\n");
    panic();
  }

  lock->cnt--;
  spinlockRelease(&lock->LOCK);
}

void spinlockCntWriteAcquire(SpinlockCnt *lock) {
  while (true) {
    spinlockAcquire(&lock->LOCK);
    if (lock->cnt == 0) {
      lock->cnt = -1;
      goto cleanup;
    }
    spinlockRelease(&lock->LOCK);
    handControl();
  }

cleanup:
  spinlockRelease(&lock->LOCK);
}

void spinlockCntWriteRelease(SpinlockCnt *lock) {
  spinlockAcquire(&lock->LOCK);
  if (lock->cnt != -1) {
    debugf("[spinlock] Something very bad is going on...\n");
    panic();
  }
  lock->cnt = 0;
  spinlockRelease(&lock->LOCK);
}

bool semaphoreWait(Semaphore *sem, uint32_t timeout) {
  uint64_t timerStart = timerTicks;
  bool     ret = false;

  while (true) {
    if (timeout > 0 && timerTicks > (timerStart + timeout))
      goto just_return; // not under any lock atm
    spinlockAcquire(&sem->LOCK);
    if (sem->cnt > 0) {
      sem->cnt--;
      goto cleanup;
    }
    spinlockRelease(&sem->LOCK);
    handControl();
  }

cleanup:
  spinlockRelease(&sem->LOCK);
just_return:
  return ret;
}

void semaphorePost(Semaphore *sem) {
  spinlockAcquire(&sem->LOCK);
  sem->cnt++;
  spinlockRelease(&sem->LOCK);
}
