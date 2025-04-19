#include <console.h>
#include <fb.h>
#include <kb.h>
#include <linked_list.h>
#include <linux.h>
#include <malloc.h>
#include <poll.h>
#include <syscalls.h>
#include <task.h>
#include <timer.h>

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
  if (!fdNode->handlers->internalPoll) {
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
    break;
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
    break;
  }
  default:
    debugf("[epoll] Unhandled opcode{%d}\n", op);
    panic(); // should've already been checked!
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
  Epoll *epoll = epollFd->dir;

  bool sigexit = false;

  // hack'y way but until I implement wake queues, it is what it is
  int    ready = 0;
  size_t target = timerTicks + timeout;
  do {
    spinlockAcquire(&epoll->LOCK_EPOLL);
    EpollWatch *browse = epoll->firstEpollWatch;
    while (browse && ready < maxevents) {
      int revents =
          browse->fd->handlers->internalPoll(browse->fd, browse->watchEvents);
      if (revents != 0 && ready < maxevents) {
        events[ready].events = revents;
        events[ready].data = browse->userlandData;
        ready++;
      }
      browse = browse->next;
    }
    spinlockRelease(&epoll->LOCK_EPOLL);

    sigexit = signalsPendingQuick(currentTask);
    if (ready > 0 || sigexit) // break immidiately!
      break;
    handControl();
  } while (timeout != 0 && (timeout == -1 || timerTicks < target));

  if (!ready && sigexit)
    return ERR(EINTR);

  return ready;
}

size_t epollPwait(OpenFile *epollFd, struct epoll_event *events, int maxevents,
                  int timeout, sigset_t *sigmask, size_t sigsetsize) {
  if (sigsetsize < sizeof(sigset_t)) {
    dbgSysFailf("weird sigset size");
    return ERR(EINVAL);
  }

  sigset_t origmask;
  if (sigmask)
    syscallRtSigprocmask(SIG_SETMASK, sigmask, &origmask, sigsetsize);
  size_t epollRet = epollWait(epollFd, events, maxevents, timeout);
  if (sigmask)
    syscallRtSigprocmask(SIG_SETMASK, &origmask, 0, sigsetsize);

  return epollRet;
}

VfsHandlers epollHandlers = {.duplicate = epollDuplicate, .close = epollClose};

// poll API
uint32_t epollToPollComp(uint32_t epoll_events) {
  uint32_t poll_events = 0;

  if (epoll_events & EPOLLIN)
    poll_events |= POLLIN;
  if (epoll_events & EPOLLOUT)
    poll_events |= POLLOUT;
  if (epoll_events & EPOLLPRI)
    poll_events |= POLLPRI;
  if (epoll_events & EPOLLERR)
    poll_events |= POLLERR;
  if (epoll_events & EPOLLHUP)
    poll_events |= POLLHUP;
#ifdef POLLRDHUP
  if (epoll_events & EPOLLRDHUP)
    poll_events |= POLLRDHUP;
#endif

  // EPOLLET and EPOLLONESHOT have no POLL equivalents, so they are ignored

  return poll_events;
}

uint32_t pollToEpollComp(uint32_t poll_events) {
  uint32_t epoll_events = 0;

  if (poll_events & POLLIN)
    epoll_events |= EPOLLIN;
  if (poll_events & POLLOUT)
    epoll_events |= EPOLLOUT;
  if (poll_events & POLLPRI)
    epoll_events |= EPOLLPRI;
  if (poll_events & POLLERR)
    epoll_events |= EPOLLERR;
  if (poll_events & POLLHUP)
    epoll_events |= EPOLLHUP;
#ifdef POLLRDHUP
  if (poll_events & POLLRDHUP)
    epoll_events |= EPOLLRDHUP;
#endif

  // No way to infer EPOLLET or EPOLLONESHOT from poll events

  return epoll_events;
}

size_t poll(struct pollfd *fds, int nfds, int timeout) {
  if (nfds < 0)
    return ERR(EINVAL);
  dbgSysExtraf("0: fd{%d} events{%d}", fds[0].fd, fds[0].events);
  int    ret = 0;
  bool   sigexit = false;
  size_t target = timerTicks + timeout;
  do {
    for (int i = 0; i < nfds; i++) {
      fds[i].revents = 0; // zero it out first
      OpenFile *fd = fsUserGetNode(currentTask, fds[i].fd);
      if (!fd)
        continue;
      if (!fd->handlers->internalPoll) {
        if (fds[i].events & POLLIN || fds[i].events & POLLOUT) {
          fds[i].revents = fds[i].events & POLLIN ? POLLIN : POLLOUT;
          ret++;
        }
        continue;
      }
      int revents = epollToPollComp(
          fd->handlers->internalPoll(fd, pollToEpollComp(fds[i].events)));
      if (revents != 0) {
        fds[i].revents = revents;
        ret++;
      }
    }

    sigexit = signalsPendingQuick(currentTask);
    if (ret > 0 || sigexit) // return immidiately!
      break;
    handControl();
  } while (timeout != 0 && (timeout == -1 || timerTicks < target));

  if (!ret && sigexit)
    return ERR(EINTR);

  return ret;
}

// i hate this obsolete system call and do not plan on making it efficient
force_inline bool selectBitmap(uint8_t *map, int index) {
  int div = index / 8;
  int mod = index % 8;
  return map[div] & (1 << mod);
}

force_inline void selectBitmapSet(uint8_t *map, int index) {
  int div = index / 8;
  int mod = index % 8;
  map[div] |= 1 << mod;
}

force_inline struct pollfd *selectAdd(struct pollfd **comp, size_t *compIndex,
                                      size_t *compLength, int fd, int events) {
  if ((*compIndex + 1) * sizeof(struct pollfd) >= *compLength) {
    *compLength *= 2;
    *comp = realloc(*comp, *compLength);
  }

  (*comp)[*compIndex].fd = fd;
  (*comp)[*compIndex].events = events;
  (*comp)[*compIndex].revents = 0;

  return &(*comp)[(*compIndex)++];
}

size_t select(int nfds, uint8_t *read, uint8_t *write, uint8_t *except,
              struct timeval *timeout) {
  size_t         compLength = sizeof(struct pollfd);
  struct pollfd *comp = (struct pollfd *)malloc(compLength);
  size_t         compIndex = 0;
  if (read) {
    for (int i = 0; i < nfds; i++) {
      if (selectBitmap(read, i))
        selectAdd(&comp, &compIndex, &compLength, i, POLLIN);
    }
  }
  if (write) {
    for (int i = 0; i < nfds; i++) {
      if (selectBitmap(write, i))
        selectAdd(&comp, &compIndex, &compLength, i, POLLOUT);
    }
  }
  if (except) {
    for (int i = 0; i < nfds; i++) {
      if (selectBitmap(except, i))
        selectAdd(&comp, &compIndex, &compLength, i, POLLPRI | POLLERR);
    }
  }

  int toZero = DivRoundUp(nfds, 8);
  if (read)
    memset(read, 0, toZero);
  if (write)
    memset(write, 0, toZero);
  if (except)
    memset(except, 0, toZero);

  size_t res = poll(
      comp, compIndex,
      timeout ? (timeout->tv_sec * 1000 + DivRoundUp(timeout->tv_usec, 1000))
              : -1);
  if (RET_IS_ERR(res)) {
    free(comp);
    return res;
  }

  size_t verify = 0;
  for (size_t i = 0; i < compIndex; i++) {
    if (!comp[i].revents)
      continue;
    if (comp[i].events & POLLIN && comp[i].revents & POLLIN) {
      selectBitmapSet(read, comp[i].fd);
      verify++;
    }
    if (comp[i].events & POLLOUT && comp[i].revents & POLLOUT) {
      selectBitmapSet(write, comp[i].fd);
      verify++;
    }
    if ((comp[i].events & POLLPRI && comp[i].revents & POLLPRI) ||
        (comp[i].events & POLLERR && comp[i].revents & POLLERR)) {
      selectBitmapSet(except, comp[i].fd);
      verify++;
    }
  }

  // nope, we need to report individual events!
  // assert(verify == res);
  // nope, POLLERR & POLLPRI don't need validation sooo yeah!
  // assert(verify >= res);
  free(comp);
  return verify;
}
