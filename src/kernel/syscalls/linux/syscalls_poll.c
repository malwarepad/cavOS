#include <linux.h>
#include <poll.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>

#define SYSCALL_EPOLL_CREATE 213
static size_t syscallEpollCreate(int size) {
  (void)size;
  return epollCreate1(0);
}

#define SYSCALL_EPOLL_WAIT 232
static size_t syscallEpollWait(int epfd, struct epoll_event *events,
                               int maxevents, int timeout) {
  OpenFile *epollFd = fsUserGetNode(currentTask, epfd);
  if (!epollFd)
    return ERR(EBADF);
  return epollWait(epollFd, events, maxevents, timeout);
}

#define SYSCALL_EPOLL_CTL 233
static size_t syscallEpollCtl(int epfd, int op, int fd,
                              struct epoll_event *event) {
  OpenFile *epollFd = fsUserGetNode(currentTask, epfd);
  if (!epollFd)
    return ERR(EBADF);
  return epollCtl(epollFd, op, fd, event);
}

#define SYSCALL_EPOLL_PWAIT 281
static size_t syscallEpollPwait(int epfd, struct epoll_event *events,
                                int maxevents, int timeout, sigset_t *sigmask,
                                size_t sigsetsize) {
  OpenFile *epollFd = fsUserGetNode(currentTask, epfd);
  if (!epollFd)
    return ERR(EBADF);
  return epollPwait(epollFd, events, maxevents, timeout, sigmask, sigsetsize);
}

#define SYSCALL_EPOLL_CREATE1 291
static size_t syscallEpollCreate1(int flags) { return epollCreate1(flags); }

void syscallsRegPoll() {
  registerSyscall(SYSCALL_EPOLL_CREATE, syscallEpollCreate);
  registerSyscall(SYSCALL_EPOLL_CREATE1, syscallEpollCreate1);
  registerSyscall(SYSCALL_EPOLL_CTL, syscallEpollCtl);
  registerSyscall(SYSCALL_EPOLL_WAIT, syscallEpollWait);
  registerSyscall(SYSCALL_EPOLL_PWAIT, syscallEpollPwait);
}
