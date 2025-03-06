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

OpenFile *unixSocketAcceptCreate(UnixSocketConn *dir) {
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
  UnixSocketConn *unixSocket = original->dir;
  spinlockAcquire(&unixSocket->LOCK_PROP);
  unixSocket->timesOpened++;
  spinlockRelease(&unixSocket->LOCK_PROP);

  return true;
}

bool unixSocketAcceptClose(OpenFile *fd) {
  spinlockAcquire(&LOCK_UNIX_CLOSE);
  UnixSocketConn *unixSocket = fd->dir;
  spinlockAcquire(&unixSocket->LOCK_PROP);
  unixSocket->timesOpened--;
  if (unixSocket->timesOpened == 0) {
    // destroy it

    // connected
    if (CONN_VALID(unixSocket->connected)) {
      spinlockAcquire(&unixSocket->connected->LOCK_SOCK);
      assert(unixSocket->connected->connected == unixSocket);
      debugf("[unix::accept::close] connected{%lx} connected_conn{%lx}\n",
             unixSocket->connected, unixSocket->connected->connected);
      unixSocket->connected->connected = CONN_DISCONNECTED;
      spinlockRelease(&unixSocket->connected->LOCK_SOCK);
    }

    // parent
    free(unixSocket->buff);
    UnixSocket *parent = unixSocket->parent;
    spinlockAcquire(&parent->LOCK_SOCK);
    assert(LinkedListRemove((void **)&parent->firstConn, unixSocket));
    spinlockRelease(&parent->LOCK_SOCK);
    spinlockRelease(&LOCK_UNIX_CLOSE);
    return true;
  }
  spinlockRelease(&unixSocket->LOCK_PROP);
  spinlockRelease(&LOCK_UNIX_CLOSE);
  return true;
}

size_t unixSocketAcceptRecvfrom(OpenFile *fd, uint8_t *out, size_t limit,
                                int flags, sockaddr_linux *addr,
                                uint32_t *len) {
  // useless unless SOCK_DGRAM
  (void)addr;
  (void)len;

  UnixSocketConn *unixConn = fd->dir;
  if (!unixConn->connected)
    return ERR(ENOTCONN);
  while (true) {
    spinlockAcquire(&unixConn->LOCK_PROP);
    if (unixConn->connected == CONN_DISCONNECTED && unixConn->posBuff == 0) {
      spinlockRelease(&unixConn->LOCK_PROP);
      return 0;
    } else if ((fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) &&
               unixConn->posBuff == 0) {
      spinlockRelease(&unixConn->LOCK_PROP);
      return ERR(EWOULDBLOCK);
    } else if (unixConn->posBuff > 0)
      break;
    spinlockRelease(&unixConn->LOCK_PROP);
  }

  // spinlock already acquired
  size_t toCopy = MIN(limit, unixConn->posBuff);
  memcpy(out, unixConn->buff, toCopy);
  unixConn->posBuff -= toCopy;
  spinlockRelease(&unixConn->LOCK_PROP);

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

  UnixSocketConn *unixConn = fd->dir;
  if (!unixConn->connected)
    return ERR(ENOTCONN);
  if (limit > unixConn->connected->sizeBuff)
    limit = unixConn->connected->sizeBuff;

  while (true) {
    if (CONN_VALID(unixConn->connected))
      spinlockAcquire(&unixConn->connected->LOCK_SOCK);
    if (unixConn->connected == CONN_DISCONNECTED) {
      spinlockRelease(&unixConn->connected->LOCK_SOCK);
      return ERR(EPIPE);
    } else if ((fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) &&
               (unixConn->connected->posBuff + limit) >
                   unixConn->connected->sizeBuff) {
      spinlockRelease(&unixConn->connected->LOCK_SOCK);
      return ERR(EWOULDBLOCK);
    } else if ((unixConn->connected->posBuff + limit) <=
               unixConn->connected->sizeBuff)
      break;
    spinlockRelease(&unixConn->connected->LOCK_SOCK);
  }

  // spinlock already acquired
  memcpy(&unixConn->connected->buff[unixConn->connected->posBuff], in, limit);
  unixConn->connected->posBuff += limit;
  spinlockRelease(&unixConn->connected->LOCK_SOCK);

  return limit;
}

size_t unixSocketAcceptWrite(OpenFile *fd, uint8_t *in, size_t limit) {
  return unixSocketAcceptSendto(fd, in, limit, 0, 0, 0);
}

size_t unixSocketOpen(void *taskPtr, int type, int protocol) {
  // rest are not supported yet, only SOCK_STREAM
  assert(type == 1);

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

  unixSocket->buff = malloc(UNIX_SOCK_BUFF_DEFAULT);
  unixSocket->sizeBuff = UNIX_SOCK_BUFF_DEFAULT;

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

  UnixSocket *sock = fd->dir;
  spinlockAcquire(&sock->LOCK_SOCK);
  sock->connMax = backlog;
  spinlockRelease(&sock->LOCK_SOCK);
  return 0;
}

size_t unixSocketAccept(OpenFile *fd, sockaddr_linux *addr, uint32_t *len) {
  UnixSocket *sock = fd->dir;
  if (addr && len && *len > 0) {
    dbgSysStubf("todo addr");
  }

  sock->accepting = true;
  while (sock->connCurr == 0) {
    if (fd->flags & O_NONBLOCK) {
      sock->acceptWouldBlock = true;
      sock->accepting = false;
      return ERR(EWOULDBLOCK);
    } else
      sock->acceptWouldBlock = false;
    handControl();
  }
  sock->accepting = false;

  // now run through the list and pick the correct thing!
  spinlockAcquire(&sock->LOCK_SOCK);
  UnixSocketConn *conn = sock->firstConn;
  while (conn) {
    if (!conn->parent) {
      // we got one that hasn't been verified yet
      sock->connCurr--;
      sock->connEstablished++;

      // race
      conn->connected->connected = conn;

      conn->parent = sock;
      conn->timesOpened = 1;
      conn->buff = malloc(UNIX_SOCK_BUFF_DEFAULT);
      conn->sizeBuff = UNIX_SOCK_BUFF_DEFAULT;
      OpenFile *acceptFd = unixSocketAcceptCreate(conn);
      spinlockRelease(&sock->LOCK_SOCK);
      return acceptFd->id;
    }
    conn = conn->next;
  }
  spinlockRelease(&sock->LOCK_SOCK);

  // should never be reached
  assert(false);
  return -1;
}

size_t unixSocketConnect(OpenFile *fd, sockaddr_linux *addr, uint32_t len) {
  UnixSocket *sock = fd->dir;
  if (sock->connMax != 0) // already ran listen()
    return ERR(ECONNREFUSED);

  if (sock->connected) // already ran connect()
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

    if (parent->bindAddr && strlength(parent->bindAddr) == safeLen &&
        memcmp(safe, parent->bindAddr, safeLen) == 0)
      break;

    parent = parent->next;
  }
  spinlockRelease(&LOCK_LL_UNIX_SOCKET);
  free(safe); // no longer needed

  // todo: actual filesystem contact
  if (!parent)
    return ERR(ENOENT);

  // nonblock edge case, check man page
  if (parent->acceptWouldBlock && fd->flags & O_NONBLOCK)
    return ERR(EINPROGRESS); // use select, poll, or epoll

  if (!parent->connMax) // listen() hasn't yet ran
    return ERR(ECONNREFUSED);

  // todo!
  assert(!(fd->flags & O_NONBLOCK));
  while (!parent->accepting) {
    handControl();
  }

  if (parent->connCurr >= parent->connMax)
    return ERR(ECONNREFUSED); // no slot

  spinlockAcquire(&parent->LOCK_SOCK);
  UnixSocketConn *conn =
      LinkedListAllocate((void **)&parent->firstConn, sizeof(UnixSocketConn));
  // conn->parent = parent; parent hasn't accepted yet
  conn->connected = sock;
  parent->connCurr++;
  spinlockRelease(&parent->LOCK_SOCK);

  // todo!
  assert(!(fd->flags & O_NONBLOCK));
  while (!conn->parent) {
    // wait for parent to accept this thing and have it's own fd on the side
    handControl();
  }

  // sock->connected = conn; done by the parent so I can avoid race conditions

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
  spinlockAcquire(&LOCK_UNIX_CLOSE);
  UnixSocket *unixSocket = fd->dir;
  debugf("[unix::sock::close] sock{%lx} connected{%lx}\n", unixSocket,
         unixSocket->connected);
  spinlockAcquire(&unixSocket->LOCK_SOCK);
  debugf("[unix::sock::close] Post-spinlock: timesOpened{%d}\n",
         unixSocket->timesOpened);
  unixSocket->timesOpened--;
  if (unixSocket->timesOpened == 0) {
    // destroy it
    if (CONN_VALID(unixSocket->connected)) {
      spinlockAcquire(&unixSocket->connected->LOCK_PROP);
      unixSocket->connected->connected = CONN_DISCONNECTED;
      spinlockRelease(&unixSocket->connected->LOCK_PROP);
    }
    free(unixSocket->buff);
    spinlockAcquire(&LOCK_LL_UNIX_SOCKET);
    assert(LinkedListRemove((void **)&firstUnixSocket, unixSocket));
    spinlockRelease(&LOCK_LL_UNIX_SOCKET);
    debugf("[unix::sock::close] Closed successfully!\n");
    spinlockRelease(&LOCK_UNIX_CLOSE);
    return true;
  }
  spinlockRelease(&unixSocket->LOCK_SOCK);
  spinlockRelease(&LOCK_UNIX_CLOSE);
  return true;
}

size_t unixSocketRecvfrom(OpenFile *fd, uint8_t *out, size_t limit, int flags,
                          sockaddr_linux *addr, uint32_t *len) {
  // useless unless SOCK_DGRAM
  (void)addr;
  (void)len;

  UnixSocket *unixSocket = fd->dir;
  if (!unixSocket->connected)
    return ERR(ENOTCONN);
  while (true) {
    spinlockAcquire(&unixSocket->LOCK_SOCK);
    if (unixSocket->connected == CONN_DISCONNECTED &&
        unixSocket->posBuff == 0) {
      spinlockRelease(&unixSocket->LOCK_SOCK);
      return 0;
    } else if ((fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) &&
               unixSocket->posBuff == 0) {
      spinlockRelease(&unixSocket->LOCK_SOCK);
      return ERR(EWOULDBLOCK);
    } else if (unixSocket->posBuff > 0)
      break;
    spinlockRelease(&unixSocket->LOCK_SOCK);
  }

  // spinlock already acquired
  size_t toCopy = MIN(limit, unixSocket->posBuff);
  memcpy(out, unixSocket->buff, toCopy);
  unixSocket->posBuff -= toCopy;
  spinlockRelease(&unixSocket->LOCK_SOCK);

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

  UnixSocket *unixSocket = fd->dir;
  if (!unixSocket->connected)
    return ERR(ENOTCONN);
  if (limit > unixSocket->connected->sizeBuff)
    limit = unixSocket->connected->sizeBuff;

  while (true) {
    if (CONN_VALID(unixSocket->connected))
      spinlockAcquire(&unixSocket->connected->LOCK_PROP);
    if (unixSocket->connected == CONN_DISCONNECTED) {
      spinlockRelease(&unixSocket->connected->LOCK_PROP);
      return ERR(EPIPE);
    } else if ((fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT) &&
               (unixSocket->connected->posBuff + limit) >
                   unixSocket->connected->sizeBuff) {
      spinlockRelease(&unixSocket->connected->LOCK_PROP);
      return ERR(EWOULDBLOCK);
    } else if ((unixSocket->connected->posBuff + limit) <=
               unixSocket->connected->sizeBuff)
      break;
    spinlockRelease(&unixSocket->connected->LOCK_PROP);
  }

  // spinlock already acquired
  memcpy(&unixSocket->connected->buff[unixSocket->connected->posBuff], in,
         limit);
  unixSocket->connected->posBuff += limit;
  spinlockRelease(&unixSocket->connected->LOCK_PROP);

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