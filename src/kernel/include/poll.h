#include "spinlock.h"
#include "task.h"
#include "types.h"
#include "vfs.h"

#ifndef POLL_H
#define POLL_H

// no lock for these two as they HAVE to pass by PollInstance's lock!
typedef struct TaskListeners {
  struct TaskListeners *next;
  Task                 *task;
} TaskListeners;

// not used. item is the key and epollEvents is the value
// of an AVL object (nvm)
typedef struct PollItem {
  struct PollItem *next;

  uint64_t key;
  int      epollEvents;
} PollItem;

typedef struct PollInstance {
  struct PollInstance *next;

  // Spinlock LOCK_INSTANCE;

  // Spinlock LOCK_POLL_INSTANCE;
  bool listening;

  TaskListeners *listeners;
  PollItem      *items; // PollItem*

  int fr;
  int haha;
} PollInstance;

PollInstance *pollRoot;
Spinlock      LOCK_POLL_ROOT;

void pollInstanceRing(size_t key, int epollEvent);

typedef struct EpollWatch {
  struct EpollWatch *next;

  OpenFile *fd;
  int       watchEvents;

  uint64_t userlandData; // pass it directly
} EpollWatch;

typedef struct Epoll {
  struct Epoll *next;

  Spinlock LOCK_EPOLL;
  int      timesOpened;

  bool legacyMode; // todo

  PollInstance *instance;

  EpollWatch *firstEpollWatch;
} Epoll;

Spinlock LOCK_LL_EPOLL;
Epoll   *firstEpoll;

size_t epollCreate1(int flags);
size_t epollCtl(OpenFile *epollFd, int op, int fd, struct epoll_event *event);
size_t epollWait(OpenFile *epollFd, struct epoll_event *events, int maxevents,
                 int timeout);
size_t epollPwait(OpenFile *epollFd, struct epoll_event *events, int maxevents,
                  int timeout, sigset_t *sigmask, size_t sigsetsize);

void epollCloseNotify(OpenFile *fd);

VfsHandlers epollHandlers;

uint32_t epollToPollComp(uint32_t epoll_events);
uint32_t pollToEpollComp(uint32_t poll_events);

size_t poll(struct pollfd *fds, int nfds, int timeout);
size_t ppoll(struct pollfd *fds, int nfds, struct timespec *timeout,
             sigset_t *sigmask, size_t sigsetsize);

size_t select(int nfds, uint8_t *read, uint8_t *write, uint8_t *except,
              struct timeval *timeout);

#endif
