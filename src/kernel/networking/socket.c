#include <linux.h>
#include <poll.h>
#include <socket.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>

#include <lwip/sockets.h>

// Lwip wrapper for userland sockets
// Copyright (C) 2025 Panagiotis

size_t socketSend(OpenFile *fd, uint8_t *out, size_t limit, int flags) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  int lwipOut = -1;
  while (true) {
    if (!(fd->handlers->internalPoll(fd, EPOLLOUT) & EPOLLOUT)) {
      if (fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) {
        lwipOut = -1;
        errno = EAGAIN;
        break;
      }
      if (signalsPendingQuick(currentTask)) {
        lwipOut = -1;
        errno = EINTR;
        break;
      }
      continue;
    }
    lwipOut = lwip_send(userSocket->lwipFd, out, limit, flags);
    if (lwipOut >= 0 || errno != EAGAIN)
      break;
  }

  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

size_t socketRecv(OpenFile *fd, uint8_t *in, size_t limit, int flags) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  int lwipOut = -1;
  while (true) {
    if (!(fd->handlers->internalPoll(fd, EPOLLIN) & EPOLLIN)) {
      if (fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) {
        lwipOut = -1;
        errno = EAGAIN;
        break;
      }
      if (signalsPendingQuick(currentTask)) {
        lwipOut = -1;
        errno = EINTR;
        break;
      }
      continue;
    }
    lwipOut = lwip_recv(userSocket->lwipFd, in, limit, flags);
    if (lwipOut >= 0 || errno != EAGAIN)
      break;
  }

  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

bool socketDuplicate(OpenFile *original, OpenFile *orphan) {
  UserSocket *userSocket = (UserSocket *)original->dir;
  spinlockAcquire(&userSocket->LOCK_INSTANCE);
  userSocket->socketInstances++;
  orphan->dir = original->dir;
  spinlockRelease(&userSocket->LOCK_INSTANCE);
  return true;
}

// don't sync fcntl() operations and keep it unblocked
void socketFcntl(OpenFile *fd, int cmd, uint64_t arg) {
  // UserSocket *userSocket = (UserSocket *)fd->dir;
  // if (cmd == F_GETFL || cmd == F_SETFL)
  //   lwip_fcntl(userSocket->lwipFd, cmd, arg);
}

bool socketClose(OpenFile *fd) {
  UserSocket *userSocket = (UserSocket *)fd->dir;
  spinlockAcquire(&userSocket->LOCK_INSTANCE);
  userSocket->socketInstances--;
  if (!userSocket->socketInstances) {
    // no more instances, actually close..
    int lwipRes = lwip_close(userSocket->lwipFd);
    if (lwipRes < 0) {
      debugf("[syscalls::socket::close] FATAL! lwipRes{%d}\n", lwipRes);
      panic();
    }
    free(userSocket);
    return true; // avoid the spinlockRelease
  }
  spinlockRelease(&userSocket->LOCK_INSTANCE);
  return true;
}

int socketInternalPoll(OpenFile *fd, int events) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  struct pollfd single = {.revents = 0,
                          .events = epollToPollComp(events),
                          .fd = userSocket->lwipFd};

  int ret = lwip_poll(&single, 1, 0);
  if (ret != 1)
    return 0;

  return pollToEpollComp(single.revents);
}

size_t socketBind(OpenFile *fd, sockaddr_linux *addr, size_t len) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  uint16_t initialFamily = sockaddrLinuxToLwip((void *)addr, len);
  int      lwipOut = lwip_bind(userSocket->lwipFd, (void *)addr, len);
  sockaddrLwipToLinux(addr, initialFamily);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

size_t socketConnect(OpenFile *fd, sockaddr_linux *addr, uint32_t len) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  uint16_t initialFamily = sockaddrLinuxToLwip((void *)addr, len);
  if (!(fd->flags & O_NONBLOCK))
    assert(lwip_fcntl(userSocket->lwipFd, F_SETFL, 0) == 0);
  int lwipOut = lwip_connect(userSocket->lwipFd, (void *)addr, len);
  if (!(fd->flags & O_NONBLOCK))
    assert(lwip_fcntl(userSocket->lwipFd, F_SETFL, O_NONBLOCK) == 0);
  sockaddrLwipToLinux(addr, initialFamily);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

size_t socketSendto(OpenFile *fd, uint8_t *buff, size_t len, int flags,
                    sockaddr_linux *dest_addr, socklen_t addrlen) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  if (!addrlen || !dest_addr)
    return socketSend(fd, buff, len, flags);

  struct sockaddr *aligned = malloc(addrlen);
  memcpy(aligned, dest_addr, addrlen);
  uint16_t initialFamily = sockaddrLinuxToLwip(aligned, addrlen);

  int lwipOut = -1;
  while (true) {
    if (!(fd->handlers->internalPoll(fd, EPOLLOUT) & EPOLLOUT)) {
      if (fd->flags & O_NONBLOCK) {
        lwipOut = -1;
        errno = EAGAIN;
        break;
      }
      if (signalsPendingQuick(currentTask)) {
        lwipOut = -1;
        errno = EINTR;
        break;
      }
      continue;
    }
    lwipOut = lwip_sendto(userSocket->lwipFd, buff, len, flags, (void *)aligned,
                          MIN(16, addrlen));
    if (lwipOut >= 0 || errno != EAGAIN)
      break;
  }

  sockaddrLwipToLinux(aligned, initialFamily);

  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

size_t socketGetsockname(OpenFile *fd, sockaddr_linux *addr, socklen_t *len) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  int lwipOut = lwip_getsockname(userSocket->lwipFd, (void *)addr, len);
  sockaddrLwipToLinux(addr, AF_INET);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

// todo: start the accept() stuff
size_t socketListen(OpenFile *fd, int backlog) {
  if (backlog == 0) // newer kernel behavior
    backlog = 1;
  if (backlog < 0)
    backlog = 128;

  UserSocket *userSocket = (UserSocket *)fd->dir;
  int         lwipOut = lwip_listen(userSocket->lwipFd, backlog);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

size_t socketRecvfrom(OpenFile *fd, uint8_t *out, size_t limit, int flags,
                      sockaddr_linux *addr, uint32_t *len) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  if (!addr || !len)
    return socketRecv(fd, out, limit, flags);

  int lwipOut = -1;
  while (true) {
    if (!(fd->handlers->internalPoll(fd, EPOLLIN) & EPOLLIN)) {
      // do a workaround cause lwip's recvfrom() is really weird sometimes
      if (fd->flags & O_NONBLOCK) {
        lwipOut = -1;
        errno = EAGAIN;
        break;
      }
      if (signalsPendingQuick(currentTask)) {
        lwipOut = -1;
        errno = EINTR;
        break;
      }
      continue;
    }
    lwipOut =
        lwip_recvfrom(userSocket->lwipFd, out, limit, flags, (void *)addr, len);
    if (lwipOut >= 0 || errno != EAGAIN)
      break;
  }

  sockaddrLwipToLinux(addr, AF_INET);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

int socketOptLevelConv(int level) {
  switch (level) {
  case 1:
    level = SOL_SOCKET;
    break;
  default:
    level = -1;
    break;
  }
  return level;
}
int socketOptOptnameConv(int optname) {
  switch (optname) {
  case 4:
    optname = SO_ERROR;
    break;
  default:
    optname = -1;
    break;
  }
  return optname;
}

size_t socketGetsockopt(OpenFile *fd, int levelLinux, int optnameLinux,
                        void *optval, uint32_t *socklen) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  int level = socketOptLevelConv(levelLinux);
  int optname = socketOptOptnameConv(optnameLinux);

  if (level < 0 || optname < 0) {
    dbgSysFailf("unsupported translation: level{%d} or optname{%d}");
    return ERR(ENOPROTOOPT);
  }

  int lwipOut =
      lwip_getsockopt(userSocket->lwipFd, level, optname, optval, socklen);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

size_t socketRecvmsg(OpenFile *fd, struct msghdr_linux *msg, int flags) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  int lwipOut = -1;
  while (true) {
    if (!(fd->handlers->internalPoll(fd, EPOLLIN) & EPOLLIN)) {
      if (fd->flags & O_NONBLOCK) {
        lwipOut = -1;
        errno = EAGAIN;
        break;
      }
      if (signalsPendingQuick(currentTask)) {
        lwipOut = -1;
        errno = EINTR;
        break;
      }
      continue;
    }
    lwipOut = lwip_recvmsg(userSocket->lwipFd, (void *)msg, flags);
    if (lwipOut >= 0 || errno != EAGAIN)
      break;
  }

  if (lwipOut >= 0 && msg->msg_name)
    sockaddrLwipToLinux(msg->msg_name, AF_INET);

  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

size_t socketReportKey(OpenFile *fd) { return 70; }

VfsHandlers socketHandlers = {.fcntl = socketFcntl,
                              .recvfrom = socketRecvfrom,
                              .recvmsg = socketRecvmsg,
                              .internalPoll = socketInternalPoll,
                              .getsockname = socketGetsockname,
                              .getsockopts = socketGetsockopt,
                              .listen = socketListen,
                              .bind = socketBind,
                              .connect = socketConnect,
                              .sendto = socketSendto,
                              .duplicate = socketDuplicate,
                              .reportKey = socketReportKey,
                              .close = socketClose};