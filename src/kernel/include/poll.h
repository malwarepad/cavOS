#include "spinlock.h"
#include "types.h"
#include "vfs.h"

#ifndef POLL_H
#define POLL_H

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

  EpollWatch *firstEpollWatch;
} Epoll;

Spinlock LOCK_LL_EPOLL;
Epoll   *firstEpoll;

size_t epollCreate1(int flags);
size_t epollCtl(OpenFile *epollFd, int op, int fd, struct epoll_event *event);
size_t epollWait(OpenFile *epollFd, struct epoll_event *events, int maxevents,
                 int timeout);

void epollCloseNotify(OpenFile *fd);

VfsHandlers epollHandlers;

#endif
