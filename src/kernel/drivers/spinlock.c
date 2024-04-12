#include <spinlock.h>

// The thing that magically makes everything thread-safe!
// Copyright (C) 2024 Panagiotis

void spinlock(SpinLock *spinlock) {
  while (spinlock->locked)
    ;

  spinlock->locked = true;
}

void spinlockRelease(SpinLock *spinlock) {
  // yeah, that's literally it
  spinlock->locked = false;
}
