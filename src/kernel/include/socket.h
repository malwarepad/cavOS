#include "spinlock.h"
#include "types.h"
#include "vfs.h"

#ifndef SOCKET_H
#define SOCKET_H

// socket.c
typedef struct UserSocket {
  Spinlock LOCK_INSTANCE;

  int socketInstances;
  int lwipFd;
} UserSocket;

VfsHandlers socketHandlers;

uint16_t sockaddrLinuxToLwip(void *dest_addr, uint32_t addrlen);
void     sockaddrLwipToLinux(void *dest_addr, uint16_t initialFamily);

// socket_v6.c
VfsHandlers socketv6Handlers;

#endif