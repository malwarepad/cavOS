#include <linux.h>
#include <poll.h>
#include <socket.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>

// Dumb IPv6 socket to return errors (fuck IPv6)
// Copyright (C) 2025 Panagiotis

size_t socketv6Connect(OpenFile *fd, sockaddr_linux *addr, uint32_t len) {
  // network unreachable
  return ERR(ENETUNREACH);
}

size_t socketv6Bind(OpenFile *fd, sockaddr_linux *addr, size_t len) {
  // cannot assign addr
  return ERR(EADDRNOTAVAIL);
}

size_t socketv6Listen(OpenFile *fd, int backlog) {
  // wasn't yet bound
  return ERR(EADDRINUSE);
}

VfsHandlers socketv6Handlers = {
    .connect = socketv6Connect, .bind = socketv6Bind, .listen = socketv6Listen};
