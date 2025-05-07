#include "arch/sys_arch.h"
#include "lwipopts.h"

#include "lwip/arch.h"
#include "lwip/opt.h"

#include <lwip/arch.h>
#include <lwip/debug.h>
#include <lwip/opt.h>
#include <lwip/stats.h>
#include <lwip/sys.h>
#include <timer.h>

#include <linked_list.h>

// lwip glue code for cavOS
// Copyright (C) 2024 Panagiotis

void sys_init(void) {
  // boo!
}

void sys_mutex_lock(Spinlock *spinlock) { spinlockAcquire(spinlock); }
void sys_mutex_unlock(Spinlock *spinlock) { spinlockRelease(spinlock); }

err_t sys_mutex_new(Spinlock *spinlock) {
  memset(spinlock, 0, sizeof(Spinlock));
  return ERR_OK;
}

err_t sys_sem_new(sys_sem_t *sem, uint8_t cnt) {
  if (cnt != 0) {
    debugf("[lwip::glue::sem::new] cnt{%d}\n", cnt);
    panic();
  }
  sem->invalid = false;
  sem->cnt = 0;
  sys_mutex_new(&sem->LOCK);

  return ERR_OK;
}

void sys_sem_signal(sys_sem_t *sem) { semaphorePost(sem); }

uint32_t sys_arch_sem_wait(sys_sem_t *sem, uint32_t timeout) {
  if (timeout) {
    debugf("[lwip::glue] todo: Timeout\n");
    panic();
  }
  bool ret = semaphoreWait(sem, timeout);
  return ret ? 0 : SYS_ARCH_TIMEOUT;
}

void sys_sem_free(sys_sem_t *sem) { sys_sem_new(sem, 0); }

void sys_sem_set_invalid(sys_sem_t *sem) {
  if (sem->invalid) {
    debugf("[lwip::glue] Already invalid!\n");
    panic();
  }

  sem->invalid = true;
}

int sys_sem_valid(sys_sem_t *sem) { return !sem->invalid; }

uint32_t sys_now(void) { return timerBootUnix * 1000 + timerTicks; }

sys_thread_t sys_thread_new(const char *pcName,
                            void (*pxThread)(void *pvParameters), void *pvArg,
                            int iStackSize, int iPriority) {
  // debugf("[lwip::glue::thread] stack{%d} name{%s}\n", iStackSize, pcName);
  Task *task = taskCreateKernel((uint64_t)pxThread, (uint64_t)pvArg);
  taskNameKernel(task, lwipCmdline, sizeof(lwipCmdline));
  return task->id;
}

err_t sys_mbox_new(sys_mbox_t *mbox, int size) {
  if (!size) {
    size = TCPIP_MBOX_SIZE;
    // debugf("Tried to create a mailbox with a size of 0!\n");
    // panic();
  }
  memset(mbox, 0, sizeof(sys_mbox_t));
  mbox->invalid = false;
  mbox->size = size;
  mbox->msges = malloc(sizeof(mbox->msges[0]) * (size + 1));
  return ERR_OK;
}

void sys_mbox_free(sys_mbox_t *mbox) {
  spinlockAcquire(&mbox->LOCK);
  assert(mbox->ptrWrite == mbox->ptrRead);
  free(mbox->msges);
  spinlockRelease(&mbox->LOCK); // for set_invalid()!
}

void sys_mbox_set_invalid(sys_mbox_t *mbox) {
  spinlockAcquire(&mbox->LOCK);
  mbox->invalid = true;
  spinlockRelease(&mbox->LOCK);
}

int sys_mbox_valid(sys_mbox_t *mbox) {
  spinlockAcquire(&mbox->LOCK);
  int ret = !mbox->invalid;
  spinlockRelease(&mbox->LOCK);
  return ret;
}

void sys_mbox_post_unsafe(sys_mbox_t *q, void *msg) {
  // spinlockAcquire(&q->LOCK);
  q->msges[q->ptrWrite] = msg;
  q->ptrWrite = (q->ptrWrite + 1) % q->size;

  mboxBlock *browse = q->firstBlock;
  while (browse) {
    mboxBlock *next = browse->next;
    if (browse->write == false) {
      browse->task->forcefulWakeupTimeUnsafe = 0;
      browse->task->state = TASK_STATE_READY;
      LinkedListRemove((void **)&q->firstBlock, browse);
    }
    browse = next;
  }

  spinlockRelease(&q->LOCK);
}

void sys_mbox_post(sys_mbox_t *q, void *msg) {
  while (true) {
    spinlockAcquire(&q->LOCK);
    if ((q->ptrWrite + 1) % q->size != q->ptrRead)
      break;
    spinlockRelease(&q->LOCK);
    // would optimize blocking here too (with the write field) but it happens so
    // rarely it's not worth it
    handControl();
  }

  sys_mbox_post_unsafe(q, msg);
}

err_t sys_mbox_trypost(sys_mbox_t *q, void *msg) {
  spinlockAcquire(&q->LOCK);
  if ((q->ptrWrite + 1) % q->size == q->ptrRead) {
    spinlockRelease(&q->LOCK);
    return ERR_MEM;
  }

  sys_mbox_post_unsafe(q, msg);
  return ERR_OK;
}

err_t sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg) {
  assert(false);
  return sys_mbox_trypost(q, msg); // xd
}

u32_t sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, u32_t timeout) {
  uint64_t timeStart = timerTicks;
  while (true) {
    spinlockAcquire(&q->LOCK);
    if (q->ptrRead != q->ptrWrite)
      break;
    // spinlockRelease(&q->LOCK);
    if (timeout && timerTicks >= (timeStart + timeout)) {
      spinlockRelease(&q->LOCK);
      return SYS_ARCH_TIMEOUT;
    }
    if (timeout)
      currentTask->forcefulWakeupTimeUnsafe = timeStart + timeout;
    mboxBlock *block =
        LinkedListAllocate((void **)&q->firstBlock, sizeof(mboxBlock));
    block->task = currentTask;
    block->write = false;
    taskSpinlockExit(currentTask, &q->LOCK);
    currentTask->state = TASK_STATE_BLOCKED;
    handControl();
    assert(!currentTask->forcefulWakeupTimeUnsafe);
  }

  // spinlockAcquire(&q->LOCK);
  *msg = q->msges[q->ptrRead];
  q->ptrRead = (q->ptrRead + 1) % q->size;
  spinlockRelease(&q->LOCK);

  return 0;
}

u32_t sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg) {
  spinlockAcquire(&q->LOCK);
  if (q->ptrRead == q->ptrWrite) {
    spinlockRelease(&q->LOCK);
    return SYS_MBOX_EMPTY;
  }

  // spinlockAcquire(&q->LOCK);
  *msg = q->msges[q->ptrRead];
  q->ptrRead = (q->ptrRead + 1) % q->size;
  spinlockRelease(&q->LOCK);

  return ERR_OK;
}

// mmm, trash
void *sio_open(u8_t devnum) {
  debugf("[lwip::glue] sio_open()\n");
  return 0;
}
u32_t sio_write(void *fd, const u8_t *data, u32_t len) {
  debugf("[lwip::glue] sio_write()\n");
  panic();
  return 0;
}
void sio_send(u8_t c, void *fd) {
  debugf("[lwip::glue] sio_send()\n");
  panic();
}
u8_t sio_recv(void *fd) {
  debugf("[lwip::glue] sio_recv()\n");
  panic();
  return 0;
}
u32_t sio_read(void *fd, u8_t *data, u32_t len) {
  debugf("[lwip::glue] sio_read()\n");
  panic();
  return 0;
}
u32_t sio_tryread(void *fd, u8_t *data, u32_t len) {
  debugf("[lwip::glue] sio_tryread()\n");
  panic();
  return 0;
}
