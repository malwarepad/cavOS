#include "spinlock.h"
#include "task.h"
#include "types.h"
#include "vfs.h"

#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H

VfsHandlers unixSocketHandlers;
VfsHandlers unixAcceptHandlers;

#define AF_UNIX 1
#define UNIX_SOCK_BUFF_DEFAULT 262144
#define UNIX_SOCK_POLL_EXTRA (4096 * 16)

#define CONN_DISCONNECTED ((void *)(size_t)(-1))

#define CONN_VALID(conn) (conn && conn != CONN_DISCONNECTED)

typedef struct UnixSocketPair {
  // common mutex
  Spinlock LOCK_PAIR;

  // accept()/server
  bool     established;
  int      serverFds;
  uint8_t *serverBuff;
  int      serverBuffPos;
  int      serverBuffSize;

  struct ucred server;
  struct ucred client;

  char *filename;

  // connect()/client
  int      clientFds;
  uint8_t *clientBuff;
  int      clientBuffPos;
  int      clientBuffSize;
} UnixSocketPair;

typedef struct UnixSocket {
  struct UnixSocket *next;

  Spinlock LOCK_SOCK;
  int      timesOpened;

  // accept()
  bool acceptWouldBlock;

  // bind()
  char *bindAddr;

  // listen()
  int              connMax; // if 0, listen() hasn't ran
  int              connCurr;
  UnixSocketPair **backlog;

  // connect()
  UnixSocketPair *pair;
} UnixSocket;

UnixSocket *firstUnixSocket;
Spinlock    LOCK_LL_UNIX_SOCKET;

// (1): we're the last ones to have access to the spinlock and the
// acceptClose() end might concurrently try to set our ->connected to a
// DISCONNECTED state

// avoids both spinlocks inside close functions waiting for eachother and it
// becoming a hardlock. check (1)
// Spinlock LOCK_UNIX_CLOSE;
// not needed with the new design, keep it as an idea

size_t unixSocketOpen(void *taskPtr, int type, int protocol);
size_t unixSocketPair(int type, int protocol, int *sv);

// "hack" since we aren't using the filename to stop bound sockets
bool unixSocketUnlinkNotify(char *filename);

#endif