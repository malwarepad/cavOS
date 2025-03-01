#include "spinlock.h"
#include "types.h"
#include "vfs.h"

#ifndef UNIX_SOCKET_H
#define UNIX_SOCKET_H

VfsHandlers unixSocketHandlers;
VfsHandlers unixAcceptHandlers;

#define AF_UNIX 1
#define UNIX_SOCK_BUFF_DEFAULT 16384

#define CONN_DISCONNECTED ((void *)(size_t)(-1))

#define CONN_VALID(conn) (conn && conn != CONN_DISCONNECTED)

typedef struct UnixSocketConn {
  struct UnixSocketConn *next;
  struct UnixSocket     *parent;
  struct UnixSocket     *connected;

  uint8_t *buff;
  int      posBuff;
  int      sizeBuff;

  Spinlock LOCK_PROP;
  int      timesOpened;
} UnixSocketConn;

typedef struct UnixSocket {
  struct UnixSocket *next;

  Spinlock LOCK_SOCK;
  int      timesOpened;

  // accept()
  bool acceptWouldBlock;
  bool accepting;

  uint8_t *buff;
  int      posBuff;
  int      sizeBuff;

  // bind()
  char *bindAddr;

  // listen()
  int             connMax; // if 0, listen() hasn't ran
  int             connEstablished;
  int             connCurr;
  UnixSocketConn *firstConn;

  // connect()
  // should make it directly r/w
  struct UnixSocketConn *connected;
} UnixSocket;

UnixSocket *firstUnixSocket;
Spinlock    LOCK_LL_UNIX_SOCKET;

// (1): we're the last ones to have access to the spinlock and the
// acceptClose() end might concurrently try to set our ->connected to a
// DISCONNECTED state

// avoids both spinlocks inside close functions waiting for eachother and it
// becoming a hardlock. check (1)
Spinlock LOCK_UNIX_CLOSE;

size_t unixSocketOpen(void *taskPtr, int type, int protocol);

#endif