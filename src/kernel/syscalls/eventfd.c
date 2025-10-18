#include <console.h>
#include <fb.h>
#include <kb.h>
#include <linux.h>
#include <malloc.h>
#include <poll.h>
#include <syscalls.h>
#include <task.h>

// Weird eventfd API (just use pipes man)
// Copyright (C) 2025 Panagiotis

typedef struct EventFd {
  uint64_t counter;

  int      utilizedBy;
  Spinlock LOCK_EVENTFD;
} EventFd;

size_t eventFdOpen(uint64_t initValue, int flags) {
  if (flags && (flags & ~(EFD_CLOEXEC | EFD_NONBLOCK)) != 0) {
    dbgSigStubf("todo flags");
    return ERR(ENOSYS);
  }

  int eventFdNumber = fsUserOpen(currentTask, "/dev/null", O_RDWR, 0);
  assert(eventFdNumber >= 0);
  OpenFile *eventFd = fsUserGetNode(currentTask, eventFdNumber);
  assert(eventFd);

  if (flags & EFD_CLOEXEC)
    eventFd->closeOnExec = true;
  if (flags & EFD_NONBLOCK)
    eventFd->flags |= O_NONBLOCK;

  EventFd *info = calloc(sizeof(EventFd), 1);
  info->counter = initValue;
  info->utilizedBy = 1;

  eventFd->dir = info;
  eventFd->handlers = &eventFdHandlers;

  return eventFdNumber;
}

size_t eventFdReportKey(OpenFile *fd) { return (size_t)fd->dir; }

bool eventFdDuplicate(OpenFile *original, OpenFile *orphan) {
  EventFd *eventFd = original->dir;
  orphan->dir = original->dir;
  spinlockAcquire(&eventFd->LOCK_EVENTFD);
  eventFd->utilizedBy++;
  spinlockRelease(&eventFd->LOCK_EVENTFD);
  return true;
}

bool eventFdClose(OpenFile *fd) {
  EventFd *eventFd = fd->dir;
  spinlockAcquire(&eventFd->LOCK_EVENTFD);
  eventFd->utilizedBy--;
  if (!eventFd->utilizedBy)
    free(eventFd);
  else
    spinlockRelease(&eventFd->LOCK_EVENTFD);
  return true;
}

size_t eventFdRead(OpenFile *fd, uint8_t *out, size_t limit) {
  if (limit < 8)
    return ERR(EINVAL);
  EventFd *eventFd = fd->dir;
  while (true) {
    spinlockAcquire(&eventFd->LOCK_EVENTFD);
    if (eventFd->counter != 0)
      break;
    if (signalsPendingQuick(currentTask)) {
      spinlockRelease(&eventFd->LOCK_EVENTFD);
      return ERR(EINTR);
    }
    if (fd->flags & O_NONBLOCK) {
      spinlockRelease(&eventFd->LOCK_EVENTFD);
      return ERR(EWOULDBLOCK);
    }
    spinlockRelease(&eventFd->LOCK_EVENTFD);
    handControl();
  }

  atomicWrite64((void *)out, eventFd->counter);
  eventFd->counter = 0;
  spinlockRelease(&eventFd->LOCK_EVENTFD);
  pollInstanceRing((size_t)fd->dir, EPOLLOUT);
  return 8;
}

size_t eventFdWrite(OpenFile *fd, uint8_t *in, size_t limit) {
  uint64_t toAdd = atomicRead64((void *)in);
  if (limit < 8 || toAdd == 0xffffffffffffffff)
    return ERR(EINVAL);
  EventFd *eventFd = fd->dir;
  while (true) {
    spinlockAcquire(&eventFd->LOCK_EVENTFD);
    if (!(toAdd > 0xffffffffffffffff - eventFd->counter))
      break;
    if (signalsPendingQuick(currentTask)) {
      spinlockRelease(&eventFd->LOCK_EVENTFD);
      return ERR(EINTR);
    }
    if (fd->flags & O_NONBLOCK) {
      spinlockRelease(&eventFd->LOCK_EVENTFD);
      return ERR(EWOULDBLOCK);
    }
    spinlockRelease(&eventFd->LOCK_EVENTFD);
    handControl();
  }

  eventFd->counter += toAdd;
  spinlockRelease(&eventFd->LOCK_EVENTFD);
  pollInstanceRing((size_t)fd->dir, EPOLLIN);
  return 8;
}

int eventFdInternalPoll(OpenFile *fd, int events) {
  EventFd *eventFd = fd->dir;
  int      revents = 0;
  spinlockAcquire(&eventFd->LOCK_EVENTFD);

  if (events & EPOLLIN && eventFd->counter > 0)
    revents |= EPOLLIN;

  if (events & EPOLLOUT && eventFd->counter < 0xffffffffffffffff)
    revents |= EPOLLOUT;

  spinlockRelease(&eventFd->LOCK_EVENTFD);
  return revents;
}

VfsHandlers eventFdHandlers = {.read = eventFdRead,
                               .write = eventFdWrite,
                               .internalPoll = eventFdInternalPoll,
                               .duplicate = eventFdDuplicate,
                               .reportKey = eventFdReportKey,
                               .close = eventFdClose};
