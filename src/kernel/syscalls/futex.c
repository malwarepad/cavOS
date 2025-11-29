#include <console.h>
#include <fb.h>
#include <kb.h>
#include <linux.h>
#include <malloc.h>
#include <paging.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>

#include <avl_tree.h>
#include <linked_list.h>

// Futex syscall for fast userspace locking
// Copyright (C) 2025 Panagiotis

typedef struct FutexAsleep {
  LLheader _ll;

  struct Futex *above;

  bool  awoken;
  Task *task;
} FutexAsleep;

typedef struct Futex {
  Spinlock LOCK_PROP;

  LLcontrol firstAsleep; // struct FutexAsleep
  // int          pid; // really it is tgid but whatever
} Futex;

// todo: find some sensible way to clean all of these up after use
AVLheader *firstFutex = 0;
Spinlock   LOCK_AVL_FUTEX = {0};

Futex *futexFind(size_t phys) {
  spinlockAcquire(&LOCK_AVL_FUTEX);
  Futex *futex = (Futex *)AVLLookup(firstFutex, phys);
  if (futex)
    goto cleanup;

  // not found, create it
  futex = calloc(sizeof(Futex), 1);
  AVLAllocate((void **)&firstFutex, phys, (avlval)futex);
  LinkedListInit(&futex->firstAsleep, sizeof(FutexAsleep));
cleanup:
  spinlockAcquire(&futex->LOCK_PROP);
  spinlockRelease(&LOCK_AVL_FUTEX);
  return futex;
}

// todo: ensure addr exists on-demand (when implemented)
size_t futexSyscall(uint32_t *addr, int op, uint32_t value,
                    struct timespec *utime, uint32_t *addr2, uint32_t value3) {
  /* Don't use currentTask here as FUTEX_WAKE is used by task exit */

  /*debugf("FUTEX! HIDE THE KIDS!! addr{%lx} op{%x} value{%d} utime{%lx} "
         "uaddr2{%lx} value3{%d}\n",
         addr, op, value, utime, addr2, value3);*/
  if (op & FUTEX_CLOCK_REALTIME) {
    debugf("[futex] Seriously, it uses FUTEX_CLOCK_REALTIME: op{%x}\n", op);
    panic();
  }

  // todo: we wont care about private rn
  // bool private = op & FUTEX_PRIVATE_FLAG;
  op &= ~FUTEX_PRIVATE_FLAG;

  size_t phys = 0;
  if (op == FUTEX_WAIT || op == FUTEX_WAKE || op == FUTEX_REQUEUE) {
    if (!addr || ((size_t)addr % 4) != 0)
      return ERR(EINVAL);
    phys = VirtualToPhysical((size_t)addr);
    assert(phys);
    dbgSysExtraf("phys{%lx}", phys);
  }

  size_t phys2 = 0;
  if (op == FUTEX_REQUEUE) {
    if (!addr2 || ((size_t)addr2 % 4) != 0)
      return ERR(EINVAL);
    phys2 = VirtualToPhysical((size_t)addr2);
    assert(phys2);
    dbgSysExtraf("phys2{%lx}", phys2);
  }

  switch (op) {
  case FUTEX_WAIT: {
    if (atomicRead32(addr) != value)
      return ERR(EAGAIN);
    size_t ret = -1;

    Futex *futex = futexFind(phys);
    // if (private)
    //   futex->pid = currentTask->pgid;

    FutexAsleep *asleep =
        LinkedListAllocate(&futex->firstAsleep, sizeof(FutexAsleep));
    asleep->above = futex;
    asleep->task = currentTask;

    // spinlockRelease(&futex->LOCK_PROP);
    taskSpinlockExit(currentTask, &futex->LOCK_PROP);
    if (utime)
      currentTask->forcefulWakeupTimeUnsafe =
          timerTicks + DivRoundUp(utime->tv_nsec, 1000000) +
          utime->tv_sec * 1000;
    currentTask->state = TASK_STATE_FUTEX;
    while (currentTask->state != TASK_STATE_READY)
      handControl();
    assert(!currentTask->forcefulWakeupTimeUnsafe);

    // figure out what happened to wake us up from our nap
    Futex *newfutex = asleep->above; // might've changed!
    spinlockAcquire(&newfutex->LOCK_PROP);
    if (asleep->awoken)
      ret = 0;
    else { // either timeout, or a pending signal
      if (signalsPendingQuick(currentTask))
        ret = ERR(EINTR);
      else
        ret = ERR(ETIMEDOUT);
    }

    // get rid of it, we are done
    assert(
        LinkedListRemove(&newfutex->firstAsleep, sizeof(FutexAsleep), asleep));
    spinlockRelease(&newfutex->LOCK_PROP);

    return ret;
    break;
  }
  case FUTEX_WAKE: {
    /* Don't use currentTask here as FUTEX_WAKE is used by task exit */
    uint8_t currentTask = 0;
    (void)currentTask;

    Futex       *futex = futexFind(phys);
    FutexAsleep *browse = (FutexAsleep *)futex->firstAsleep.firstObject;
    int          awokenCnt = 0;

    while (browse) {
      if (!browse->awoken && awokenCnt < value) {
        // wake it up
        browse->task->forcefulWakeupTimeUnsafe = 0;
        browse->task->state = TASK_STATE_READY;
        browse->awoken = true;
        awokenCnt++; // ack
      }
      browse = (FutexAsleep *)browse->_ll.next;
    }
    spinlockRelease(&futex->LOCK_PROP);

    return awokenCnt;
    break;
  }
  case FUTEX_REQUEUE: {
    Futex *futex = futexFind(phys);
    Futex *futex2 = futexFind(phys2);

    FutexAsleep *browse = (FutexAsleep *)futex->firstAsleep.firstObject;
    int          awokenCnt = 0;
    int          movedCnt = 0;

    while (browse) {
      if (!browse->awoken && awokenCnt < value) {
        // wake it up
        browse->task->forcefulWakeupTimeUnsafe = 0;
        browse->task->state = TASK_STATE_READY;
        browse->awoken = true;
        awokenCnt++; // ack
      } else if (!browse->awoken && movedCnt < (uint32_t)(size_t)(utime)) {
        // move those
        movedCnt++;
        FutexAsleep *next = (FutexAsleep *)browse->_ll.next;
        assert(LinkedListUnregister(&futex->firstAsleep, sizeof(FutexAsleep),
                                    browse));
        browse->_ll.next = 0;
        browse->above = futex2;
        // LinkedListPushFrontUnsafe((void **)&futex2->firstAsleep, browse);
        FutexAsleep *b = (FutexAsleep *)futex2->firstAsleep.firstObject;
        if (!b)
          futex2->firstAsleep.firstObject = (LLheader *)browse;
        else {
          while (b) {
            if (!b->_ll.next)
              break;
            b = (FutexAsleep *)b->_ll.next;
          }
          b->_ll.next = (void *)browse;
        }
        browse = next;
        continue;
      }
      browse = (FutexAsleep *)browse->_ll.next;
    }
    spinlockRelease(&futex2->LOCK_PROP);
    spinlockRelease(&futex->LOCK_PROP);

    return awokenCnt + movedCnt;
    break;
  }
  default:
    debugf("[futex] Invalid operation{%x}\n", op);
    panic();
    return ERR(ENOSYS);
    break;
  }

  debugf("[futex] Dead-end. It requires some serious effort to end up here!\n");
  assert(false);
}
