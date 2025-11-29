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

// todo! THIS IS USING ->ll directly!!! consider re-doing some stuff

// for tmp exclusions
#include <socket.h>

// Polling APIs & kernel helper utility
// Copyright (C) 2025 Panagiotis

// poll instance helpers
void pollInstanceDestroy(PollInstance *instance, bool lockAcquired) {
  if (!lockAcquired)
    spinlockAcquire(&LOCK_POLL_ROOT);

  if ((PollInstance *)dsPollRoot.firstObject == instance)
    dsPollRoot.firstObject = instance->_ll.next;
  else {
    PollInstance *browse = (PollInstance *)dsPollRoot.firstObject;
    // todo: maybe add some sort of assertion to this
    while (browse && browse->_ll.next) {
      if ((PollInstance *)browse->_ll.next == instance) {
        browse->_ll.next = instance->_ll.next;
        break;
      }
      browse = (PollInstance *)browse->_ll.next;
    }
  }

  LinkedListDestroy(&instance->listeners, sizeof(TaskListeners));
  LinkedListDestroy(&instance->items, sizeof(PollItem));
  free(instance);

  if (!lockAcquired)
    spinlockRelease(&LOCK_POLL_ROOT);
}

bool pollItemLookupCb(void *data, void *ctx) {
  PollItem *item = data;
  uint64_t  key = *(uint64_t *)ctx;
  return item->key == key;
}

PollItem *pollItemLookup(PollInstance *instance, uint64_t key) {
  return LinkedListSearch(&instance->items, pollItemLookupCb, &key);
}

void pollItemAdd(PollInstance *instance, uint64_t key, int epollEvents) {
  PollItem *target = calloc(sizeof(PollItem), 1);
  target->key = key;
  target->epollEvents = epollEvents;
  LinkedListPushFrontUnsafe(&instance->items, target);
}

void pollItemRemove(PollInstance *instance, uint64_t key) {
  PollItem *target = pollItemLookup(instance, key);
  assert(target);
  assert(LinkedListRemove(&instance->items, sizeof(PollItem), target));
}

PollInstance *pollInstanceAllocate() {
  PollInstance *target = LinkedListAllocate(&dsPollRoot, sizeof(PollInstance));
  LinkedListInit(&target->listeners, sizeof(TaskListeners));
  LinkedListInit(&target->items, sizeof(PollItem));
  return target;
}

void pollInstanceRingInner(PollInstance *instanceUnsafe, bool involuntary) {
  // notify all of them, removing them one-by-one
  TaskListeners *listener =
      (TaskListeners *)instanceUnsafe->listeners.firstObject;
  while (listener) {
    if (!involuntary && listener->task->state == TASK_STATE_READY) {
      // involuntarily woken up first
      assert(listener->task->extras & EXTRAS_INVOLUTARY_WAKEUP);
      return; // will be ringed by the other thing anyways
      // todo: ensure we won't be deleted first (future, still valid)
      continue;
    }
    // assert(listener->task->state != TASK_STATE_READY);
    //   on ^ case, continue
    // todo: safe wakeup w/asserts on task.c
    // maybe not needed since the lock is released...
    assert(!listener->task->spinlockQueueEntry);
    listener->task->forcefulWakeupTimeUnsafe = 0;
    listener->task->state = TASK_STATE_READY;
    TaskListeners *next = (TaskListeners *)listener->_ll.next;
    free(listener);
    listener = next;
  }

  // get ready for next fair (+ the previous free())
  instanceUnsafe->listeners.firstObject = 0;

  // the next one will go through locks and find it dead
  instanceUnsafe->listening = false;
}

// At the moment pollInstanceRing() basically "ignores" epollEvent. You still
// need to fill it in correctly though. That is because atm there isn't a
// clear-cut solution for stuff like sockets with different states so it serves
// more so as a loop restrictor for stuff like (e/p)poll. In the future that
// won't be the case and more restrictions will be applied.

// Same issue as unix sockets applies w/ptmx

// Another important issue is networking, where I'm not sure how I wanna
// approach polling & lwip. Atm I'm doing the simplest most straightforward
// thing which is 1 poll/schedule when a networking socket gets found in the
// list, as networking requirements are light enough to tolerate this. It will
// need to be re-evaluated as a whole though.

// Yes, these are notes to my future self.

// make SURE this is OUT OF LOCKS USED IN internalPoll() or smth!
void pollInstanceRing(size_t key, int epollEvent) {
  spinlockAcquire(&LOCK_POLL_ROOT); // <- very important thing
  PollInstance *instance = (PollInstance *)dsPollRoot.firstObject;
  while (instance) {
    // pollItemLookup(instance->items, epollEvent)
    // (epollEvent & value || (epollEvent & ~(EPOLLIN | EPOLLOUT)))
    PollItem *item = pollItemLookup(instance, key);
    if (instance->listening && item)
      pollInstanceRingInner(instance, false);

    instance = (PollInstance *)instance->_ll.next;
  }
  spinlockRelease(&LOCK_POLL_ROOT);
}

// before make SURE to check list WITH LOCK ACQUIRED!
void pollInstanceWait(PollInstance *instance, size_t expiry) {
#if !NO_LEGACY_POLL
  spinlockRelease(&LOCK_POLL_ROOT);
  handControl();
  return;
#endif

  // instance IS locked AND READY
  // spinlockAcquire(&LOCK_POLL_ROOT);
  // spinlockAcquire(&instance->LOCK_POLL_INSTANCE);
  TaskListeners *listener =
      LinkedListAllocate(&instance->listeners, sizeof(TaskListeners));
  listener->task = currentTask;
  if (instance->listening)
    debugf("[poll::instance] WARN: Experiemental! > 1 listener bound!\n");
  instance->listening = true;
  if (expiry)
    currentTask->forcefulWakeupTimeUnsafe = expiry;
  taskSpinlockExit(currentTask, &LOCK_POLL_ROOT);
  currentTask->extras &= ~EXTRAS_INVOLUTARY_WAKEUP; // clear
  currentTask->state = TASK_STATE_BLOCKED;
  handControl();
  assert(!currentTask->forcefulWakeupTimeUnsafe);

  if (currentTask->extras & EXTRAS_INVOLUTARY_WAKEUP) {
    spinlockAcquire(&LOCK_POLL_ROOT);
    if (instance->listening) // no race conds here!
      pollInstanceRingInner(instance, true);
    spinlockRelease(&LOCK_POLL_ROOT);
  }
}

// epoll API
size_t epollCreate1(int flags) {
  size_t epollFd = fsUserOpen(currentTask, "/dev/null", O_RDWR, 0);
  assert(!RET_IS_ERR(epollFd));

  OpenFile *epollNode = fsUserGetNode(currentTask, epollFd);
  assert(epollNode);

  if (flags & EPOLL_CLOEXEC)
    epollNode->closeOnExec = true;

  spinlockAcquire(&LOCK_LL_EPOLL);
  Epoll *epoll = LinkedListAllocate(&dsEpoll, sizeof(Epoll));
  spinlockRelease(&LOCK_LL_EPOLL);
  epoll->timesOpened = 1;
  spinlockAcquire(&LOCK_POLL_ROOT);
  epoll->instance = pollInstanceAllocate();
  spinlockRelease(&LOCK_POLL_ROOT);

  epollNode->handlers = &epollHandlers;
  epollNode->dir = epoll;
  LinkedListInit(&epoll->firstEpollWatch, sizeof(EpollWatch));

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
    pollInstanceDestroy(epoll->instance, true);
    assert(LinkedListRemove(&dsEpoll, sizeof(Epoll), epoll));
    spinlockRelease(&LOCK_LL_EPOLL);
    return true;
  }
  spinlockRelease(&epoll->LOCK_EPOLL);
  return true;
}

void epollCloseNotify(OpenFile *fd) {
  spinlockAcquire(&LOCK_LL_EPOLL);

  Epoll *browseEpoll = (Epoll *)dsEpoll.firstObject;
  while (browseEpoll) {
    spinlockAcquire(&browseEpoll->LOCK_EPOLL);

    EpollWatch *browseEpollWatch =
        (EpollWatch *)browseEpoll->firstEpollWatch.firstObject;
    while (browseEpollWatch) {
      if (browseEpollWatch->fd == fd)
        break;
      browseEpollWatch = (EpollWatch *)browseEpollWatch->_ll.next;
    }
    if (browseEpollWatch) {
      // we found it!
      // todo: get rid of it! (edge case regardless)
      // todo: probable "leak" with ->instance
      LinkedListRemove(&browseEpoll->firstEpollWatch, sizeof(EpollWatch),
                       browseEpollWatch);
      spinlockRelease(&browseEpoll->LOCK_EPOLL);
      spinlockRelease(&LOCK_LL_EPOLL);
      return;
    }

    spinlockRelease(&browseEpoll->LOCK_EPOLL);
    browseEpoll = (Epoll *)browseEpoll->_ll.next;
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
  spinlockAcquire(&LOCK_POLL_ROOT);

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
  assert(fdNode->handlers->reportKey && fdNode->handlers->reportKey(fdNode));

  switch (op) {
  case EPOLL_CTL_ADD: {
    assert(fdNode->handlers != &socketHandlers); // todo legacyMode
    EpollWatch *epollWatch =
        LinkedListAllocate(&epoll->firstEpollWatch, sizeof(EpollWatch));
    epollWatch->fd = fdNode;
    epollWatch->watchEvents = event->events;
    epollWatch->userlandData = event->data;

    pollItemAdd(epoll->instance, fdNode->handlers->reportKey(fdNode),
                event->events);
    break;
  }
  case EPOLL_CTL_MOD: {
    EpollWatch *browse = (EpollWatch *)epoll->firstEpollWatch.firstObject;
    while (browse) {
      if (browse->fd == fdNode)
        break;
      browse = (EpollWatch *)browse->_ll.next;
    }
    if (!browse) {
      ret = ERR(ENOENT);
      goto cleanup;
    }
    browse->watchEvents = event->events;
    browse->userlandData = event->data;
    pollItemRemove(epoll->instance, fdNode->handlers->reportKey(fdNode));
    pollItemAdd(epoll->instance, fdNode->handlers->reportKey(fdNode),
                event->events);
    break;
  }
  case EPOLL_CTL_DEL: {
    EpollWatch *browse = (EpollWatch *)epoll->firstEpollWatch.firstObject;
    while (browse) {
      if (browse->fd == fdNode)
        break;
      browse = (EpollWatch *)browse->_ll.next;
    }
    if (!browse) {
      ret = ERR(ENOENT);
      goto cleanup;
    }
    pollItemRemove(epoll->instance, fdNode->handlers->reportKey(fdNode));
    assert(
        LinkedListRemove(&epoll->firstEpollWatch, sizeof(EpollWatch), browse));
    break;
  }
  default:
    debugf("[epoll] Unhandled opcode{%d}\n", op);
    panic(); // should've already been checked!
    break;
  }

cleanup:
  spinlockRelease(&LOCK_POLL_ROOT);
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
    spinlockAcquire(&LOCK_POLL_ROOT); // these two for pollInstanceWait()
    EpollWatch *browse = (EpollWatch *)epoll->firstEpollWatch.firstObject;
    while (browse && ready < maxevents) {
      int revents =
          browse->fd->handlers->internalPoll(browse->fd, browse->watchEvents);
      if (revents != 0 && ready < maxevents) {
        events[ready].events = revents;
        events[ready].data = browse->userlandData;
        ready++;
      }
      browse = (EpollWatch *)browse->_ll.next;
    }
    spinlockRelease(&epoll->LOCK_EPOLL);

    sigexit = signalsPendingQuick(currentTask);
    if (ready > 0 || sigexit) { // break immidiately!
      spinlockRelease(&LOCK_POLL_ROOT);
      break;
    }
    if (timeout != 0)
      pollInstanceWait(epoll->instance, timeout == -1 ? 0 : target);
    else {
      spinlockRelease(&LOCK_POLL_ROOT);
    }
    // handControl();
  } while (timeout != 0 && (timeout == -1 || timerTicks < target));

  // todo (later): check that this is correct for signals
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

  spinlockAcquire(&LOCK_POLL_ROOT);
  PollInstance *instance = pollInstanceAllocate();
  spinlockRelease(&LOCK_POLL_ROOT);
  bool first = false;
  bool legacy = false; // contains networking items

  do {
    spinlockAcquire(&LOCK_POLL_ROOT);
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
      // if (!fd->handlers->reportKey) {
      //   asm volatile("cli");
      //   while (1)
      //     asm volatile("hlt");
      // }
      assert(fd->handlers->reportKey && fd->handlers->reportKey(fd));
      int revents = epollToPollComp(
          fd->handlers->internalPoll(fd, pollToEpollComp(fds[i].events)));
      if (revents != 0) {
        fds[i].revents = revents;
        ret++;
      }

      if (!first) {
        if (fd->handlers == &socketHandlers)
          legacy = true;
        size_t    key = fd->handlers->reportKey(fd);
        PollItem *existing = pollItemLookup(instance, key);
        if (existing) {
          // existing->epollEvents |= fds[i].events;
          pollItemRemove(instance, key);
          pollItemAdd(instance, key, fds[i].events);
        } else
          pollItemAdd(instance, key, fds[i].events);
      }
    }
    first = true;

    sigexit = signalsPendingQuick(currentTask);
    if (ret > 0 || sigexit) { // return immidiately!
      spinlockRelease(&LOCK_POLL_ROOT);
      break;
    }
    if (timeout != 0 && !legacy)
      pollInstanceWait(instance, timeout == -1 ? 0 : target);
    else {
      spinlockRelease(&LOCK_POLL_ROOT);
      if (legacy)
        handControl();
    }
    // handControl();
  } while (timeout != 0 && (timeout == -1 || timerTicks < target));

  pollInstanceDestroy(instance, false);

  if (!ret && sigexit)
    return ERR(EINTR);

  return ret;
}

size_t ppoll(struct pollfd *fds, int nfds, struct timespec *timeout,
             sigset_t *sigmask, size_t sigsetsize) {
  if (sigsetsize < sizeof(sigset_t)) {
    dbgSysFailf("weird sigset size");
    return ERR(EINVAL);
  }

  sigset_t origmask;
  if (sigmask)
    syscallRtSigprocmask(SIG_SETMASK, sigmask, &origmask, sigsetsize);
  size_t epollRet = poll(
      fds, nfds,
      timeout ? (DivRoundUp(timeout->tv_nsec, 1000000) + timeout->tv_sec * 1000)
              : -1);
  if (sigmask)
    syscallRtSigprocmask(SIG_SETMASK, &origmask, 0, sigsetsize);

  return epollRet;
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
