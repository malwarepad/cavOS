#include "system.h"
#include "types.h"
#include "util.h"

#ifndef MALLOC_GLUE_H
#define MALLOC_GLUE_H

// malloc.c provides it's own headers for a lot of stuff apparently!

// spinlocks
typedef Spinlock MLOCK_T;

MLOCK_T malloc_global_mutex;
int     ACQUIRE_LOCK(Spinlock *lock);
int     RELEASE_LOCK(Spinlock *lock);
int     INITIAL_LOCK(Spinlock *lock);

#endif