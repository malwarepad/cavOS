#include <malloc_glue.h>
#include <system.h>
#include <util.h>
#include <vmm.h>

#define DEBUG_DLMALLOC_GLUE 0

void *last = 0;
void *sbrk(long increment) {
#if DEBUG_DLMALLOC_GLUE
  debugf("[dlmalloc::sbrk] size{%lx}\n", increment);
#endif
  if (increment < 0)
    return 0; // supposed to release, whatever.
  if (!increment)
    return last;
  // return 0;

  uint64_t blocks = DivRoundUp(increment, BLOCK_SIZE);
  void    *virt = VirtualAllocate(blocks);
  memset(virt, 0, blocks * BLOCK_SIZE);

  last = (void *)((size_t)virt + increment);

#if DEBUG_DLMALLOC_GLUE
  debugf("[dlmalloc::sbrk] found{%lx}\n", virt);
#endif
  return virt;
}

int  __errnoF = 0;
int *__errno_location = &__errnoF;
// void __errno() { return &__errnoF; }

void abort() {
  debugf("[dlmalloc::abort] errno{%x}!\n", __errnoF);
  panic();
}

// spinlocks/mutexes/whatever people call them; I truly don't care!
MLOCK_T malloc_global_mutex = ATOMIC_FLAG_INIT;

int ACQUIRE_LOCK(Spinlock *lock) {
  spinlockAcquire(lock);
  return 0;
}

int RELEASE_LOCK(Spinlock *lock) {
  spinlockRelease(lock);
  return 0;
}

int INITIAL_LOCK(Spinlock *lock) {
  RELEASE_LOCK(lock);
  return 0;
}
