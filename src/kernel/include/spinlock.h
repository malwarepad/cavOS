#include "types.h"

#ifndef SPINLOCK_H
#define SPINLOCK_H

typedef struct SpinLock {
  bool locked;
} SpinLock;
#define __SPINLOCK(name) static SpinLock name = {.locked = false}

#endif
