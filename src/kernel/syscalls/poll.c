#include <console.h>
#include <fb.h>
#include <kb.h>
#include <linked_list.h>
#include <linux.h>
#include <malloc.h>
#include <poll.h>
#include <syscalls.h>
#include <task.h>

// Polling APIs & kernel helper utility
// Copyright (C) 2025 Panagiotis

// epoll API
size_t epollCreate1(int flags) {
  size_t epollFd = fsUserOpen(currentTask, "/dev/null", O_RDWR, 0);
  assert(!RET_IS_ERR(epollFd));

  OpenFile *epollNode = fsUserGetNode(currentTask, epollFd);
  assert(epollNode);

  if (flags & EPOLL_CLOEXEC)
    epollNode->closeOnExec = true;

  spinlockAcquire(&LOCK_LL_EPOLL);
  Epoll *epoll = LinkedListAllocate((void **)&firstEpoll, sizeof(Epoll));
  spinlockRelease(&LOCK_LL_EPOLL);
  epoll->timesOpened = 1;

  epollNode->handlers = &epollHandlers;
  epollNode->dir = epoll;

  assert(epollFd == epollNode->id);
  return epollNode->id;
}

bool epollDuplicate(OpenFile *original, OpenFile *orphan) {
  orphan->dir = original->dir;
  Epoll *epoll = original->dir;
  spinlockAcquire(&epoll->LOCK_EPOLL);
  epoll->timesOpened++;
  spinlockRelease(&epoll->LOCK_EPOLL);

  return true;
}

bool epollClose(OpenFile *fd) {
  Epoll *epoll = fd->dir;
  spinlockAcquire(&epoll->LOCK_EPOLL);
  epoll->timesOpened--;
  if (epoll->timesOpened == 0) {
    // destroy this thing
    // todo! interest list cleanup, watchlist(s)
    spinlockAcquire(&LOCK_LL_EPOLL);
    assert(LinkedListRemove((void **)(&firstEpoll), epoll));
    spinlockRelease(&LOCK_LL_EPOLL);
    return true;
  }
  spinlockRelease(&epoll->LOCK_EPOLL);
  return true;
}

void epollCloseNotify(OpenFile *fd) {
  spinlockAcquire(&LOCK_LL_EPOLL);

  Epoll *browseEpoll = firstEpoll;
  while (browseEpoll) {
    spinlockAcquire(&browseEpoll->LOCK_EPOLL);

    EpollWatch *browseEpollWatch = browseEpoll->firstEpollWatch;
    while (browseEpollWatch) {
      if (browseEpollWatch->fd == fd)
        break;
      browseEpollWatch = browseEpollWatch->next;
    }
    if (browseEpollWatch) {
      // we found it!
      LinkedListRemove((void **)(&browseEpoll->firstEpollWatch),
                       browseEpollWatch);
      spinlockRelease(&browseEpoll->LOCK_EPOLL);
      spinlockRelease(&LOCK_LL_EPOLL);
      return;
    }

    spinlockRelease(&browseEpoll->LOCK_EPOLL);
    browseEpoll = browseEpoll->next;
  }

  spinlockRelease(&LOCK_LL_EPOLL);
}

size_t epollCtl(OpenFile *epollFd, int op, int fd, struct epoll_event *event) {
  if (op != EPOLL_CTL_ADD && op != EPOLL_CTL_DEL && op != EPOLL_CTL_MOD)
    return ERR(EINVAL);
  if (op & EPOLLET || op & EPOLLONESHOT || op & EPOLLEXCLUSIVE) {
    dbgSysStubf("bad opcode!"); // could atl do oneshot, but later
    return ERR(ENOSYS);
  }

  Epoll *epoll = epollFd->dir;

  size_t ret = 0;
  spinlockAcquire(&epoll->LOCK_EPOLL);

  OpenFile *fdNode = fsUserGetNode(currentTask, fd);
  if (!fdNode) {
    ret = ERR(EBADF);
    goto cleanup;
  }

  // todo
  if (!fdNode->handlers->internalPoll || !fdNode->handlers->addWatchlist) {
    ret = ERR(EPERM);
    goto cleanup;
  }

  switch (op) {
  case EPOLL_CTL_ADD: {
    EpollWatch *epollWatch = LinkedListAllocate(
        (void **)(&epoll->firstEpollWatch), sizeof(EpollWatch));
    epollWatch->fd = fdNode;
    epollWatch->watchEvents = event->events;
    epollWatch->userlandData = event->data;
    break;
  }
  case EPOLL_CTL_MOD: {
    EpollWatch *browse = epoll->firstEpollWatch;
    while (browse) {
      if (browse->fd == fdNode)
        break;
      browse = browse->next;
    }
    if (!browse) {
      ret = ERR(ENOENT);
      goto cleanup;
    }
    browse->watchEvents = event->events;
    browse->userlandData = event->data;
  }
  case EPOLL_CTL_DEL: {
    EpollWatch *browse = epoll->firstEpollWatch;
    while (browse) {
      if (browse->fd == fdNode)
        break;
      browse = browse->next;
    }
    if (!browse) {
      ret = ERR(ENOENT);
      goto cleanup;
    }
    assert(LinkedListRemove((void **)(&epoll->firstEpollWatch), browse));
  }
  default:
    debugf("[epoll] Unhandled opcode{%d}\n", op);
    panic();
    break;
  }

cleanup:
  spinlockRelease(&epoll->LOCK_EPOLL);
  return ret;
}

size_t epollWait(OpenFile *epollFd, struct epoll_event *events, int maxevents,
                 int timeout) {
  if (maxevents < 1)
    return ERR(EINVAL);

  Epoll      *epoll = epollFd->dir;
  EpollWatch *browse = epoll->firstEpollWatch;
  while (browse) {
    debugf("[x] %lx\n", browse->fd);
    browse = browse->next;
  }

  // panic(); // todo
  return ERR(ENOSYS);
}

VfsHandlers epollHandlers = {.duplicate = epollDuplicate, .close = epollClose};
