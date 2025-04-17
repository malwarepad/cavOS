#include <linked_list.h>
#include <malloc.h>
#include <syscalls.h>
#include <task.h>
#include <unixSocket.h>
#include <util.h>

#include <lwip/sockets.h>

// AF_UNIX socket implementation (still needs a lot of testing)
// Copyright (C) 2025 Panagiotis

OpenFile *unixSocketAcceptCreate(UnixSocketPair *dir) {
  size_t sockFd = fsUserOpen(currentTask, "/dev/null", O_RDWR, 0);
  assert(!RET_IS_ERR(sockFd));

  OpenFile *sockNode = fsUserGetNode(currentTask, sockFd);
  assert(sockNode);

  sockNode->dir = dir;
  sockNode->handlers = &unixAcceptHandlers;

  return sockNode;
}

bool unixSocketAcceptDuplicate(OpenFile *original, OpenFile *orphan) {
  orphan->dir = original->dir;
  UnixSocketPair *pair = original->dir;
  spinlockAcquire(&pair->LOCK_PAIR);
  pair->serverFds++;
  spinlockRelease(&pair->LOCK_PAIR);

  return true;
}

UnixSocketPair *unixSocketAllocatePair() {
  UnixSocketPair *pair = calloc(sizeof(UnixSocketPair), 1);
  pair->clientBuffSize = UNIX_SOCK_BUFF_DEFAULT;
  pair->serverBuffSize = UNIX_SOCK_BUFF_DEFAULT;
  pair->serverBuff = malloc(pair->serverBuffSize);
  pair->clientBuff = malloc(pair->clientBuffSize);
  return pair;
}

void unixSocketFreePair(UnixSocketPair *pair) {
  assert(pair->serverFds == 0 && pair->clientFds == 0);
  free(pair->clientBuff);
  free(pair->serverBuff);
  free(pair->filename);
  free(pair);
}

bool unixSocketAcceptClose(OpenFile *fd) {
  UnixSocketPair *pair = fd->dir;
  spinlockAcquire(&pair->LOCK_PAIR);
  pair->serverFds--;

  if (pair->serverFds == 0 && pair->clientFds == 0)
    unixSocketFreePair(pair);
  else
    spinlockRelease(&pair->LOCK_PAIR);

  return true;
}

size_t unixSocketAcceptRecvfrom(OpenFile *fd, uint8_t *out, size_t limit,
                                int flags, sockaddr_linux *addr,
                                uint32_t *len) {
  // useless unless SOCK_DGRAM
  (void)addr;
  (void)len;

  UnixSocketPair *pair = fd->dir;
  if (!pair->clientFds && pair->serverBuffPos == 0)
    return 0;
  while (true) {
    spinlockAcquire(&pair->LOCK_PAIR);
    if (!pair->clientFds && pair->serverBuffPos == 0) {
      spinlockRelease(&pair->LOCK_PAIR);
      return 0;
    } else if ((fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) &&
               pair->serverBuffPos == 0) {
      spinlockRelease(&pair->LOCK_PAIR);
      return ERR(EWOULDBLOCK);
    } else if (pair->serverBuffPos > 0)
      break;
    spinlockRelease(&pair->LOCK_PAIR);
  }

  // spinlock already acquired
  size_t toCopy = MIN(limit, pair->serverBuffPos);
  memcpy(out, pair->serverBuff, toCopy);
  memmove(pair->serverBuff, &pair->serverBuff[toCopy],
          pair->serverBuffPos - toCopy);
  pair->serverBuffPos -= toCopy;
  spinlockRelease(&pair->LOCK_PAIR);

  return toCopy;
}

size_t unixSocketAcceptSendto(OpenFile *fd, uint8_t *in, size_t limit,
                              int flags, sockaddr_linux *addr, uint32_t len) {
  // useless unless SOCK_DGRAM
  (void)addr;
  (void)len;

  UnixSocketPair *pair = fd->dir;
  if (limit > pair->clientBuffSize) {
    debugf("[socket] Warning! Truncating limit{%ld} to clientBuffSize{%ld}\n",
           limit, pair->clientBuffSize);
    limit = pair->clientBuffSize;
  }

  while (true) {
    spinlockAcquire(&pair->LOCK_PAIR);
    if (!pair->clientFds) {
      spinlockRelease(&pair->LOCK_PAIR);
      atomicBitmapSet(&currentTask->sigPendingList, SIGPIPE);
      return ERR(EPIPE);
    } else if ((fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) &&
               (pair->clientBuffPos + limit) > pair->clientBuffSize) {
      spinlockRelease(&pair->LOCK_PAIR);
      return ERR(EWOULDBLOCK);
    } else if ((pair->clientBuffPos + limit) <= pair->clientBuffSize)
      break;
    spinlockRelease(&pair->LOCK_PAIR);
  }

  // spinlock already acquired
  memcpy(&pair->clientBuff[pair->clientBuffPos], in, limit);
  pair->clientBuffPos += limit;
  spinlockRelease(&pair->LOCK_PAIR);

  return limit;
}

int unixSocketAcceptInternalPoll(OpenFile *fd, int events) {
  UnixSocketPair *pair = fd->dir;
  int             revents = 0;

  spinlockAcquire(&pair->LOCK_PAIR);
  if (!pair->clientFds)
    revents |= EPOLLHUP;

  if (events & EPOLLOUT && pair->clientFds &&
      pair->clientBuffPos < pair->clientBuffSize)
    revents |= EPOLLOUT;

  if (events & EPOLLIN && pair->serverBuffPos > 0)
    revents |= EPOLLIN;
  spinlockRelease(&pair->LOCK_PAIR);

  return revents;
}

size_t unixSocketOpen(void *taskPtr, int type, int protocol) {
  // rest are not supported yet, only SOCK_STREAM
  if (!(type & 1)) {
    dbgSysStubf("unsupported type{%x}", type);
    return ERR(ENOSYS);
  }

  Task  *task = (Task *)taskPtr;
  size_t sockFd = fsUserOpen(task, "/dev/null", O_RDWR, 0);
  assert(!RET_IS_ERR(sockFd));

  OpenFile *sockNode = fsUserGetNode(task, sockFd);
  assert(sockNode);

  sockNode->handlers = &unixSocketHandlers;
  spinlockAcquire(&LOCK_LL_UNIX_SOCKET);
  UnixSocket *unixSocket =
      LinkedListAllocate((void **)&firstUnixSocket, sizeof(UnixSocket));
  spinlockRelease(&LOCK_LL_UNIX_SOCKET);
  sockNode->dir = unixSocket;

  unixSocket->timesOpened = 1;

  if (type | SOCK_CLOEXEC)
    sockNode->closeOnExec = true;
  // if (type | SOCK_NONBLOCK)
  //   sockNode->flags |= O_NONBLOCK;

  return sockFd;
}

char *unixSocketAddrSafe(sockaddr_linux *addr, size_t len) {
  if (addr->sa_family != AF_UNIX)
    return (void *)ERR(EAFNOSUPPORT);

  size_t addrLen = len - sizeof(addr->sa_family);
  if (addrLen <= 0)
    return (void *)ERR(EINVAL);

  char *unsafe = calloc(addrLen + 1, 1);     // ENSURE there's a null char
  bool  abstract = addr->sa_data[0] == '\0'; // todo: not all sockets!
  int   skip = abstract ? 1 : 0;
  memcpy(unsafe, &addr->sa_data[skip], addrLen - skip);
  spinlockAcquire(&currentTask->infoFs->LOCK_FS);
  char *safe = fsSanitize(currentTask->infoFs->cwd, unsafe);
  spinlockRelease(&currentTask->infoFs->LOCK_FS);
  free(unsafe);

  return safe;
}

bool unixSocketUnlinkNotify(char *filename) {
  size_t len = strlength(filename);
  spinlockAcquire(&LOCK_LL_UNIX_SOCKET);
  UnixSocket *browse = firstUnixSocket;
  while (browse) {
    spinlockAcquire(&browse->LOCK_SOCK);
    if (browse->bindAddr && strlength(browse->bindAddr) == len &&
        memcmp(browse->bindAddr, filename, len) == 0)
      break;
    spinlockRelease(&browse->LOCK_SOCK);
    browse = browse->next;
  }
  spinlockRelease(&LOCK_LL_UNIX_SOCKET);

  if (browse) {
    // spinlock already here!
    browse->bindAddr = 0;
    spinlockRelease(&browse->LOCK_SOCK);
  }

  return !!browse;
}

size_t unixSocketBind(OpenFile *fd, sockaddr_linux *addr, size_t len) {
  UnixSocket *sock = fd->dir;
  if (sock->bindAddr)
    return ERR(EINVAL);

  // sanitize the filename
  char *safe = unixSocketAddrSafe(addr, len);
  if (RET_IS_ERR((size_t)(safe)))
    return (size_t)safe;
  dbgSysExtraf("safe{%s}", safe);

  // check if it already exists
  struct stat statTarg = {0};
  bool        ret = fsStatByFilename(currentTask, safe, &statTarg);
  if (ret) {
    free(safe);
    if (!(statTarg.st_mode & S_IFSOCK))
      return ERR(ENOTSOCK);
    else
      return ERR(EADDRINUSE);
  }

  // make sure there are no duplicates
  size_t safeLen = strlength(safe);
  spinlockAcquire(&LOCK_LL_UNIX_SOCKET);
  UnixSocket *browse = firstUnixSocket;
  while (browse) {
    spinlockAcquire(&browse->LOCK_SOCK);
    if (browse->bindAddr && strlength(browse->bindAddr) == safeLen &&
        memcmp(safe, browse->bindAddr, safeLen) == 0)
      break;
    spinlockRelease(&browse->LOCK_SOCK);
    browse = browse->next;
  }
  spinlockRelease(&LOCK_LL_UNIX_SOCKET);

  // found a duplicate!
  if (browse) {
    spinlockRelease(&browse->LOCK_SOCK);
    free(safe);
    return ERR(EADDRINUSE);
  }

  sock->bindAddr = safe;
  return 0;
}

size_t unixSocketListen(OpenFile *fd, int backlog) {
  if (backlog == 0) // newer kernel behavior
    backlog = 1;
  if (backlog < 0)
    backlog = 128;

  // maybe do a typical array here
  UnixSocket *sock = fd->dir;
  spinlockAcquire(&sock->LOCK_SOCK);
  sock->connMax = backlog;
  sock->backlog = calloc(sock->connMax * sizeof(UnixSocketPair *), 1);
  spinlockRelease(&sock->LOCK_SOCK);
  return 0;
}

size_t unixSocketAccept(OpenFile *fd, sockaddr_linux *addr, uint32_t *len) {
  UnixSocket *sock = fd->dir;
  if (addr && len && *len > 0) {
    dbgSysStubf("todo addr");
  }

  while (true) {
    spinlockAcquire(&sock->LOCK_SOCK);
    if (sock->connCurr > 0)
      break;
    if (fd->flags & O_NONBLOCK) {
      sock->acceptWouldBlock = true;
      spinlockRelease(&sock->LOCK_SOCK);
      return ERR(EWOULDBLOCK);
    } else
      sock->acceptWouldBlock = false;
    spinlockRelease(&sock->LOCK_SOCK);
    handControl();
  }

  // now pick the first thing! (sock spinlock already engaged)
  UnixSocketPair *pair = sock->backlog[0];
  spinlockAcquire(&pair->LOCK_PAIR);
  assert(pair->serverFds == 0);
  pair->serverFds++;
  pair->established = true;
  pair->filename = strdup(sock->bindAddr);
  spinlockRelease(&pair->LOCK_PAIR);

  OpenFile *acceptFd = unixSocketAcceptCreate(pair);
  sock->backlog[0] = 0; // just in case
  memmove(sock->backlog, &sock->backlog[1],
          (sock->connMax - 1) * sizeof(UnixSocketPair *));
  sock->connCurr--;
  spinlockRelease(&sock->LOCK_SOCK);
  return acceptFd->id;
}

size_t unixSocketConnect(OpenFile *fd, sockaddr_linux *addr, uint32_t len) {
  UnixSocket *sock = fd->dir;
  if (sock->connMax != 0) // already ran listen()
    return ERR(ECONNREFUSED);

  if (sock->pair) // already ran connect()
    return ERR(EISCONN);

  char *safe = unixSocketAddrSafe(addr, len);
  if (RET_IS_ERR((size_t)(safe)))
    return (size_t)safe;
  size_t safeLen = strlength(safe);
  dbgSysExtraf("safe{%s}", safe);

  // find object
  spinlockAcquire(&LOCK_LL_UNIX_SOCKET);
  UnixSocket *parent = firstUnixSocket;
  while (parent) {
    if (parent == sock) {
      parent = parent->next;
      continue;
    }

    spinlockAcquire(&parent->LOCK_SOCK);
    if (parent->bindAddr && strlength(parent->bindAddr) == safeLen &&
        memcmp(safe, parent->bindAddr, safeLen) == 0)
      break;
    spinlockRelease(&parent->LOCK_SOCK);

    parent = parent->next;
  }
  spinlockRelease(&LOCK_LL_UNIX_SOCKET);
  free(safe); // no longer needed

  // todo: actual filesystem contact
  if (!parent)
    return ERR(ENOENT);

  // nonblock edge case, check man page
  if (parent->acceptWouldBlock && fd->flags & O_NONBLOCK) {
    spinlockRelease(&parent->LOCK_SOCK);
    return ERR(EINPROGRESS); // use select, poll, or epoll
  }

  // listen() hasn't yet ran
  if (!parent->connMax) {
    spinlockRelease(&parent->LOCK_SOCK);
    return ERR(ECONNREFUSED);
  }

  // todo!
  assert(!(fd->flags & O_NONBLOCK));
  // spinlockAcquire(&parent->LOCK_SOCK);
  if (parent->connCurr >= parent->connMax) {
    spinlockRelease(&parent->LOCK_SOCK);
    return ERR(ECONNREFUSED); // no slot
  }
  UnixSocketPair *pair = unixSocketAllocatePair();
  sock->pair = pair;
  pair->clientFds = 1;
  parent->backlog[parent->connCurr++] = pair;
  spinlockRelease(&parent->LOCK_SOCK);

  // todo!
  assert(!(fd->flags & O_NONBLOCK));
  while (true) {
    spinlockAcquire(&pair->LOCK_PAIR);
    if (pair->established)
      break;
    spinlockRelease(&pair->LOCK_PAIR);
    // wait for parent to accept this thing and have it's own fd on the side
    handControl();
  }
  spinlockRelease(&pair->LOCK_PAIR);

  return 0;
}

bool unixSocketDuplicate(OpenFile *original, OpenFile *orphan) {
  orphan->dir = original->dir;
  UnixSocket *unixSocket = original->dir;
  spinlockAcquire(&unixSocket->LOCK_SOCK);
  unixSocket->timesOpened++;
  if (unixSocket->pair) {
    spinlockAcquire(&unixSocket->pair->LOCK_PAIR);
    unixSocket->pair->clientFds++;
    spinlockRelease(&unixSocket->pair->LOCK_PAIR);
  }
  spinlockRelease(&unixSocket->LOCK_SOCK);

  return true;
}

bool unixSocketClose(OpenFile *fd) {
  UnixSocket *unixSocket = fd->dir;
  spinlockAcquire(&unixSocket->LOCK_SOCK);
  unixSocket->timesOpened--;
  if (unixSocket->pair) {
    spinlockAcquire(&unixSocket->pair->LOCK_PAIR);
    unixSocket->pair->clientFds--;
    if (!unixSocket->pair->clientFds && !unixSocket->pair->serverFds)
      unixSocketFreePair(unixSocket->pair);
    else
      spinlockRelease(&unixSocket->pair->LOCK_PAIR);
  }
  if (unixSocket->timesOpened == 0) {
    // destroy it
    spinlockAcquire(&LOCK_LL_UNIX_SOCKET);
    assert(LinkedListRemove((void **)&firstUnixSocket, unixSocket));
    spinlockRelease(&LOCK_LL_UNIX_SOCKET);
    return true;
  }
  spinlockRelease(&unixSocket->LOCK_SOCK);
  return true;
}

size_t unixSocketRecvfrom(OpenFile *fd, uint8_t *out, size_t limit, int flags,
                          sockaddr_linux *addr, uint32_t *len) {
  // useless unless SOCK_DGRAM
  (void)addr;
  (void)len;

  UnixSocket     *socket = fd->dir;
  UnixSocketPair *pair = socket->pair;
  if (!pair)
    return ERR(ENOTCONN);
  if (!pair->serverFds && pair->clientBuffPos == 0)
    return 0;
  while (true) {
    spinlockAcquire(&pair->LOCK_PAIR);
    if (!pair->serverFds && pair->clientBuffPos == 0) {
      spinlockRelease(&pair->LOCK_PAIR);
      return 0;
    } else if ((fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) &&
               pair->clientBuffPos == 0) {
      spinlockRelease(&pair->LOCK_PAIR);
      return ERR(EWOULDBLOCK);
    } else if (pair->clientBuffPos > 0)
      break;
    spinlockRelease(&pair->LOCK_PAIR);
  }

  // spinlock already acquired
  size_t toCopy = MIN(limit, pair->clientBuffPos);
  memcpy(out, pair->clientBuff, toCopy);
  memmove(pair->clientBuff, &pair->clientBuff[toCopy],
          pair->clientBuffPos - toCopy);
  pair->clientBuffPos -= toCopy;
  spinlockRelease(&pair->LOCK_PAIR);

  return toCopy;
}

size_t unixSocketSendto(OpenFile *fd, uint8_t *in, size_t limit, int flags,
                        sockaddr_linux *addr, uint32_t len) {
  // useless unless SOCK_DGRAM
  (void)addr;
  (void)len;

  UnixSocket     *socket = fd->dir;
  UnixSocketPair *pair = socket->pair;
  if (!pair)
    return ERR(ENOTCONN);
  if (limit > pair->serverBuffSize) {
    debugf("[socket] Warning! Truncating limit{%ld} to serverBuffSize{%ld}\n",
           limit, pair->serverBuffSize);
    limit = pair->serverBuffSize;
  }

  while (true) {
    spinlockAcquire(&pair->LOCK_PAIR);
    if (!pair->serverFds) {
      spinlockRelease(&pair->LOCK_PAIR);
      atomicBitmapSet(&currentTask->sigPendingList, SIGPIPE);
      return ERR(EPIPE);
    } else if ((fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) &&
               (pair->serverBuffPos + limit) > pair->serverBuffSize) {
      spinlockRelease(&pair->LOCK_PAIR);
      return ERR(EWOULDBLOCK);
    } else if ((pair->serverBuffPos + limit) <= pair->serverBuffSize)
      break;
    spinlockRelease(&pair->LOCK_PAIR);
  }

  // spinlock already acquired
  memcpy(&pair->serverBuff[pair->serverBuffPos], in, limit);
  pair->serverBuffPos += limit;
  spinlockRelease(&pair->LOCK_PAIR);

  return limit;
}

size_t unixSocketGetpeername(OpenFile *fd, sockaddr_linux *addr,
                             uint32_t *len) {
  UnixSocket     *socket = fd->dir;
  UnixSocketPair *pair = socket->pair;
  if (!pair)
    return ERR(ENOTCONN);

  int actualLen = sizeof(addr->sa_family) + strlength(pair->filename);
  int toCopy = MIN(*len, actualLen);
  if (toCopy < sizeof(addr->sa_family)) // you're POOR!
    return ERR(EINVAL);
  addr->sa_family = AF_UNIX;
  memcpy(addr->sa_data, pair->filename, toCopy - sizeof(addr->sa_family));
  *len = toCopy;
  return 0;
}

size_t unixSocketRecvmsg(OpenFile *fd, struct msghdr_linux *msg, int flags) {
  if (msg->msg_name || msg->msg_namelen > 0) {
    dbgSysStubf("todo optional addr");
    return ERR(ENOSYS);
  }
  msg->msg_controllen = 0;
  msg->msg_flags = 0;
  size_t cnt = 0;
  bool   noblock = flags & MSG_DONTWAIT;
  for (int i = 0; i < msg->msg_iovlen; i++) {
    struct iovec *curr =
        (struct iovec *)((size_t)msg->msg_iov + i * sizeof(struct iovec));
    if (cnt > 0 && fd->handlers->internalPoll) {
      // check syscalls_fs.c for why this is necessary
      if (!(fd->handlers->internalPoll(fd, EPOLLIN) & EPOLLIN))
        return cnt;
    }
    size_t singleCnt = unixSocketRecvfrom(fd, curr->iov_base, curr->iov_len,
                                          noblock ? MSG_DONTWAIT : 0, 0, 0);
    if (RET_IS_ERR(singleCnt))
      return singleCnt;

    cnt += singleCnt;
  }

  return cnt;
}

size_t unixSocketAcceptRecvmsg(OpenFile *fd, struct msghdr_linux *msg,
                               int flags) {
  if (msg->msg_name || msg->msg_namelen > 0) {
    dbgSysStubf("todo optional addr");
    return ERR(ENOSYS);
  }
  msg->msg_controllen = 0;
  msg->msg_flags = 0;
  size_t cnt = 0;
  bool   noblock = flags & MSG_DONTWAIT;
  for (int i = 0; i < msg->msg_iovlen; i++) {
    struct iovec *curr =
        (struct iovec *)((size_t)msg->msg_iov + i * sizeof(struct iovec));
    if (cnt > 0 && fd->handlers->internalPoll) {
      // check syscalls_fs.c for why this is necessary
      if (!(fd->handlers->internalPoll(fd, EPOLLIN) & EPOLLIN))
        return cnt;
    }
    size_t singleCnt = unixSocketAcceptRecvfrom(
        fd, curr->iov_base, curr->iov_len, noblock ? MSG_DONTWAIT : 0, 0, 0);
    if (RET_IS_ERR(singleCnt))
      return singleCnt;

    cnt += singleCnt;
  }

  return cnt;
}

int unixSocketInternalPoll(OpenFile *fd, int events) {
  UnixSocket *socket = fd->dir;
  int         revents = 0;

  if (socket->connMax > 0) {
    // listen()
    spinlockAcquire(&socket->LOCK_SOCK);
    if (socket->connCurr < socket->connMax) // can connect()
      revents |= (events & EPOLLOUT) ? EPOLLOUT : 0;
    if (socket->connCurr > 0) // can accept()
      revents |= (events & EPOLLIN) ? EPOLLIN : 0;
    spinlockRelease(&socket->LOCK_SOCK);
  } else if (socket->pair) {
    // connect()
    UnixSocketPair *pair = socket->pair;
    spinlockAcquire(&pair->LOCK_PAIR);
    if (!pair->serverFds)
      revents |= EPOLLHUP;

    if (events & EPOLLOUT && pair->serverFds &&
        pair->serverBuffPos < pair->serverBuffSize)
      revents |= EPOLLOUT;

    if (events & EPOLLIN && pair->clientBuffPos > 0)
      revents |= EPOLLIN;
    spinlockRelease(&pair->LOCK_PAIR);
  } else
    revents |= EPOLLHUP;

  return revents;
}

size_t unixSocketPair(int type, int protocol, int *sv) {
  size_t sock1 = unixSocketOpen(currentTask, type, protocol);
  if (RET_IS_ERR(sock1))
    return sock1;

  OpenFile *sock1Fd = fsUserGetNode(currentTask, sock1);
  assert(sock1Fd);

  UnixSocketPair *pair = unixSocketAllocatePair();
  pair->clientFds = 1;
  pair->serverFds = 1;

  UnixSocket *sock = sock1Fd->dir;
  sock->pair = pair;

  OpenFile *sock2Fd = unixSocketAcceptCreate(pair);

  // finish it off
  sv[0] = sock1Fd->id;
  sv[1] = sock2Fd->id;

  dbgSysExtraf("fds{%d, %d}", sv[0], sv[1]);
  return 0;
}

VfsHandlers unixSocketHandlers = {.sendto = unixSocketSendto,
                                  .recvfrom = unixSocketRecvfrom,
                                  .bind = unixSocketBind,
                                  .listen = unixSocketListen,
                                  .accept = unixSocketAccept,
                                  .connect = unixSocketConnect,
                                  .getpeername = unixSocketGetpeername,
                                  .recvmsg = unixSocketRecvmsg,
                                  .duplicate = unixSocketDuplicate,
                                  .close = unixSocketClose,
                                  .internalPoll = unixSocketInternalPoll};

VfsHandlers unixAcceptHandlers = {.sendto = unixSocketAcceptSendto,
                                  .recvfrom = unixSocketAcceptRecvfrom,
                                  .recvmsg = unixSocketAcceptRecvmsg,
                                  .duplicate = unixSocketAcceptDuplicate,
                                  .close = unixSocketAcceptClose,
                                  .internalPoll = unixSocketAcceptInternalPoll};