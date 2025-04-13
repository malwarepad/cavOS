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
size_t epollPwait(OpenFile *epollFd, struct epoll_event *events, int maxevents,
                  int timeout, sigset_t *sigmask, size_t sigsetsize);

void epollCloseNotify(OpenFile *fd);

VfsHandlers epollHandlers;

uint32_t epollToPollComp(uint32_t epoll_events);
uint32_t pollToEpollComp(uint32_t poll_events);

size_t poll(struct pollfd *fds, int nfds, int timeout);

size_t select(int nfds, uint8_t *read, uint8_t *write, uint8_t *except,
              struct timeval *timeout);

#endif
