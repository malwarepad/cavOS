#include <linked_list.h>
#include <malloc.h>
#include <syscalls.h>
#include <task.h>
#include <unixSocket.h>
#include <util.h>

#include <lwip/sockets.h>

// AF_UNIX socket implementation (still needs a lot of testing)
// Copyright (C) 2025 Panagiotis

#define DEBUG_SOCK 0
#if !DEBUG_SOCK
#define debugf(...) ((void)0)
#endif

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
    return ERR(ENOTCONN);
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

size_t unixSocketAcceptRead(OpenFile *fd, uint8_t *out, size_t limit) {
  return unixSocketAcceptRecvfrom(fd, out, limit, 0, 0, 0);
}

size_t unixSocketAcceptSendto(OpenFile *fd, uint8_t *in, size_t limit,
                              int flags, sockaddr_linux *addr, uint32_t len) {
  // useless unless SOCK_DGRAM
  (void)addr;
  (void)len;

  UnixSocketPair *pair = fd->dir;
  if (!pair->clientFds)
    return ERR(ENOTCONN);
  if (limit > pair->clientBuffSize)
    limit = pair->clientBuffSize;

  while (true) {
    spinlockAcquire(&pair->LOCK_PAIR);
    if (!pair->clientFds) {
      spinlockRelease(&pair->LOCK_PAIR);
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

size_t unixSocketAcceptWrite(OpenFile *fd, uint8_t *in, size_t limit) {
  return unixSocketAcceptSendto(fd, in, limit, 0, 0, 0);
}

size_t unixSocketOpen(void *taskPtr, int type, int protocol) {
  // rest are not supported yet, only SOCK_STREAM
  if (type != 1) {
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

  return sockFd;
}

size_t unixSocketBind(OpenFile *fd, sockaddr_linux *addr, size_t len) {
  if (addr->sa_family != AF_UNIX)
    return ERR(EAFNOSUPPORT);

  size_t addrLen = len - sizeof(addr->sa_family) + 1;
  if (addrLen <= 1)
    return ERR(EINVAL);

  UnixSocket *sock = fd->dir;
  if (sock->bindAddr)
    return ERR(EINVAL);

  // sanitize the filename
  char *unsafe = malloc(addrLen);
  memcpy(unsafe, addr->sa_data, addrLen);
  char *safe = fsSanitize(currentTask->cwd, unsafe);
  free(unsafe);

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
  spinlockRelease(&pair->LOCK_PAIR);

  OpenFile *acceptFd = unixSocketAcceptCreate(pair);
  memmove(&sock->backlog[1], sock->backlog,
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

  if (addr->sa_family != AF_UNIX)
    return ERR(EAFNOSUPPORT);

  // find the length
  int addrLen = len - sizeof(addr->sa_family) + 1;
  if (addrLen <= 1)
    return ERR(EINVAL);

  // sanitize properly
  char *unsafe = malloc(addrLen);
  memcpy(unsafe, addr->sa_data, addrLen);
  char *safe = fsSanitize(currentTask->cwd, unsafe);
  free(unsafe);
  int safeLen = strlength(safe);

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
  spinlockRelease(&unixSocket->LOCK_SOCK);

  return true;
}

bool unixSocketClose(OpenFile *fd) {
  UnixSocket *unixSocket = fd->dir;
  debugf("[unix::sock::close] sock{%lx} connected{%lx}\n", unixSocket,
         unixSocket->connected);
  spinlockAcquire(&unixSocket->LOCK_SOCK);
  debugf("[unix::sock::close] Post-spinlock: timesOpened{%d}\n",
         unixSocket->timesOpened);
  unixSocket->timesOpened--;
  if (unixSocket->timesOpened == 0) {
    // destroy it
    spinlockAcquire(&LOCK_LL_UNIX_SOCKET);
    assert(LinkedListRemove((void **)&firstUnixSocket, unixSocket));
    spinlockRelease(&LOCK_LL_UNIX_SOCKET);
    debugf("[unix::sock::close] Closed successfully!\n");
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
  if (!pair || (!pair->serverFds && pair->clientBuffPos == 0))
    return ERR(ENOTCONN);
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

size_t unixSocketRead(OpenFile *fd, uint8_t *out, size_t limit) {
  return unixSocketRecvfrom(fd, out, limit, 0, 0, 0);
}

size_t unixSocketSendto(OpenFile *fd, uint8_t *in, size_t limit, int flags,
                        sockaddr_linux *addr, uint32_t len) {
  // useless unless SOCK_DGRAM
  (void)addr;
  (void)len;

  UnixSocket     *socket = fd->dir;
  UnixSocketPair *pair = socket->pair;
  if (!pair || !pair->serverFds)
    return ERR(ENOTCONN);
  if (limit > pair->serverBuffSize)
    limit = pair->serverBuffSize;

  while (true) {
    spinlockAcquire(&pair->LOCK_PAIR);
    if (!pair->serverFds) {
      spinlockRelease(&pair->LOCK_PAIR);
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

size_t unixSocketWrite(OpenFile *fd, uint8_t *in, size_t limit) {
  return unixSocketSendto(fd, in, limit, 0, 0, 0);
}

VfsHandlers unixSocketHandlers = {.read = unixSocketRead,
                                  .write = unixSocketWrite,
                                  .sendto = unixSocketSendto,
                                  .recvfrom = unixSocketRecvfrom,
                                  .bind = unixSocketBind,
                                  .listen = unixSocketListen,
                                  .accept = unixSocketAccept,
                                  .connect = unixSocketConnect,
                                  .duplicate = unixSocketDuplicate,
                                  .close = unixSocketClose};

VfsHandlers unixAcceptHandlers = {.read = unixSocketAcceptRead,
                                  .write = unixSocketAcceptWrite,
                                  .sendto = unixSocketAcceptSendto,
                                  .recvfrom = unixSocketAcceptRecvfrom,
                                  .duplicate = unixSocketAcceptDuplicate,
                                  .close = unixSocketAcceptClose};