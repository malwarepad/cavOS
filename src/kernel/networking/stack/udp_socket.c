#include <nic_controller.h>
#include <poll.h>
#include <syscalls.h>
#include <udp.h>
#include <vfs.h>

// Userspace socket bindings for the UDP protocol
// Copyright (C) 2025 Panagiotis

// sock needs to be locked beforehand
void netConnectionUdpOpen(UdpSocket *sock, uint16_t localPort, uint8_t *addr,
                          uint16_t remotePort) {
  UdpStore *store = &selectedNIC->udp;
  spinlockAcquire(&store->LOCK_LL_UDP_CONNS);
  if (!sock->conn) {
    sock->conn =
        LinkedListAllocate(&store->dsUdpConnections, sizeof(UdpConnection));
    LinkedListInit(&sock->conn->dsBuffers, sizeof(udpBuffer));
  }
  if (addr)
    memcpy(sock->conn->ip, addr, IPv4_BYTE_SIZE);
  if (!sock->conn->localPort) // localPort can be 0 but still bound() properly
    sock->conn->localPort =
        localPort ? localPort : netPortsGen(&selectedNIC->udp.netPortsUdp);
  if (remotePort)
    sock->conn->remotePort = remotePort;

  if (sock->conn->ip[0])
    debugf("[net::udp::sock] CONNECT TO %d.%d.%d.%d:%d\n", sock->conn->ip[0],
           sock->conn->ip[1], sock->conn->ip[2], sock->conn->ip[3],
           sock->conn->remotePort);
  else
    debugf("[net::udp::sock] ACCEPTING TO US AT %d\n", sock->conn->localPort);
  spinlockRelease(&store->LOCK_LL_UDP_CONNS);
}

void netConnectionUdpBuffWipeCb(void *data, void *ctx) {
  udpBuffer *buff = data;
  free(buff->buff);
}

void netConnectionUdpClose(UdpConnection *conn, UdpStore *store) {
  uint16_t localPort = 0;

  // get inside the connection
  spinlockAcquire(&store->LOCK_LL_UDP_CONNS);
  localPort = conn->localPort;

  // free all the remaining buffers
  LinkedListTraverse(&conn->dsBuffers, netConnectionUdpBuffWipeCb, 0);
  LinkedListDestroy(&conn->dsBuffers, sizeof(udpBuffer));

  // remove the connection
  assert(
      LinkedListRemove(&store->dsUdpConnections, sizeof(UdpConnection), conn));
  spinlockRelease(&store->LOCK_LL_UDP_CONNS);

  // mark the local port as free
  assert(localPort);
  netPortsFree(&store->netPortsUdp, localPort);
}

extern VfsHandlers udpSocketHandlers;

void netSocketUdpInit(void *_fd) {
  OpenFile *fd = _fd;
  fd->handlers = &udpSocketHandlers;

  UdpSocket *sock = calloc(sizeof(UdpSocket), 1);
  sock->timesOpened = 1;
  fd->dir = sock;
}

bool netSocketUdpDuplicate(OpenFile *original, OpenFile *orphan) {
  UdpSocket *sock = (UdpSocket *)original->dir;
  spinlockAcquire(&sock->LOCK_SOCKET);
  sock->timesOpened++;
  orphan->dir = original->dir;
  spinlockRelease(&sock->LOCK_SOCKET);
  return true;
}

bool netSocketUdpClose(OpenFile *fd) {
  UdpSocket *sock = fd->dir;
  spinlockAcquire(&sock->LOCK_SOCKET);
  sock->timesOpened--;
  if (sock->timesOpened == 0) {
    if (sock->conn)
      netConnectionUdpClose(sock->conn, &selectedNIC->udp);
    free(sock);
  } else
    spinlockRelease(&sock->LOCK_SOCKET);
  return true;
}

size_t netSocketUdpBind(OpenFile *fd, sockaddr_linux *addr, size_t len) {
  assert(len == sizeof(sockaddr_in_linux));
  sockaddr_in_linux *sockaddr = (sockaddr_in_linux *)addr;

  UdpSocket *sock = fd->dir;
  spinlockAcquire(&sock->LOCK_SOCKET);
  if (sock->conn) // already bind()ed and/or connect()ed
    return ERR(EINVAL);

  uint16_t localPort = switch_endian_16(sockaddr->sin_port);
  if (localPort &&
      !netPortsMarkSafe(&selectedNIC->udp.netPortsUdp, localPort)) {
    spinlockRelease(&sock->LOCK_SOCKET);
    return ERR(EADDRINUSE);
  }

  netConnectionUdpOpen(sock, localPort, 0, 0);
  spinlockRelease(&sock->LOCK_SOCKET);
  return 0;
}

size_t netSocketUdpConnect(OpenFile *fd, sockaddr_linux *addr, uint32_t len) {
  assert(len == sizeof(sockaddr_in_linux));
  sockaddr_in_linux *sockaddr = (sockaddr_in_linux *)addr;

  UdpSocket *sock = fd->dir;
  spinlockAcquire(&sock->LOCK_SOCKET);
  if (sock->connected) {
    spinlockRelease(&sock->LOCK_SOCKET);
    return ERR(EISCONN);
  }

  netConnectionUdpOpen(sock, 0, sockaddr->sin_addr,
                       switch_endian_16(sockaddr->sin_port));
  sock->connected = true;
  spinlockRelease(&sock->LOCK_SOCKET);
  return 0;
}

size_t netSocketUdpSendto(OpenFile *fd, uint8_t *buff, size_t len, int flags,
                          sockaddr_linux *dest_addr, uint32_t addrlen) {
  // !bug! dest_addr NEEDS to be ignored if the socket is connected!
  UdpSocket *sock = fd->dir;

  uint8_t  finalAddr[IPv4_BYTE_SIZE];
  uint16_t localPort = 0, remotePort = 0;
  if (dest_addr && addrlen) {
    assert(addrlen == sizeof(sockaddr_in_linux));
    sockaddr_in_linux *dest_addr_casted = (sockaddr_in_linux *)dest_addr;
    memcpy(finalAddr, dest_addr_casted->sin_addr, IPv4_BYTE_SIZE);
    remotePort = switch_endian_16(dest_addr_casted->sin_port);
  }

  spinlockAcquire(&sock->LOCK_SOCKET);
  if (!sock->conn) { // we do an ephemeral bind() for it
    debugf("[net::udp::sock] Doing the automatic bind. Really now?\n");
    netConnectionUdpOpen(sock, 0, 0, 0);
  }
  spinlockAcquire(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
  localPort = sock->conn->localPort;
  if (!remotePort) // dest_addr
    remotePort = sock->conn->remotePort;
  if (!finalAddr[0]) // dest_addr
    memcpy(finalAddr, sock->conn->ip, IPv4_BYTE_SIZE);
  spinlockRelease(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
  spinlockRelease(&sock->LOCK_SOCKET);

  if (!finalAddr[0]) // probable if we depend on a (nonexistent) dest_addr
    return ERR(EDESTADDRREQ);

  // finally do the request
  size_t finalLen = NET_UDP_CARRY_BARE + sizeof(udpHeader) + len;
  if (finalLen > selectedNIC->mtu)
    return ERR(EMSGSIZE);
  uint8_t *final = malloc(finalLen);
  netIPv4InitBuffer(final, finalLen);

  udpHeader *udp = NET_UDP(final);
  udp->checksum = 0;
  udp->length = switch_endian_16(sizeof(udpHeader) + len);
  udp->srcPort = switch_endian_16(localPort);
  udp->destPort = switch_endian_16(remotePort);

  memcpy(&final[finalLen - len], buff, len);
  netIPv4Send(selectedNIC, final, finalLen, IPV4_PROTOCOL_UDP, finalAddr);
  free(final);

  return len;
}

#define MSG_DONTWAIT 0x08
size_t netSocketUdpRecvfrom(OpenFile *fd, uint8_t *out, size_t limit, int flags,
                            sockaddr_linux *addr, uint32_t *len) {
  UdpSocket *sock = fd->dir;
  spinlockAcquire(&sock->LOCK_SOCKET);
  if (!sock->conn) {
    spinlockRelease(&sock->LOCK_SOCKET);
    return ERR(EINVAL);
  }

  udpBuffer *object = 0;
  while (true) {
    spinlockAcquire(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
    object = LinkedListSearchFirst(&sock->conn->dsBuffers);
    if (!object) {
      spinlockRelease(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
      spinlockRelease(&sock->LOCK_SOCKET);
      if (signalsPendingQuick(currentTask))
        return ERR(EINTR);
      if (fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT)
        return ERR(EWOULDBLOCK);
      else
        pollIndependentAwait(fd, EPOLLIN);
    } else
      break;
  }
  assert(
      LinkedListUnregister(&sock->conn->dsBuffers, sizeof(udpBuffer), object));
  sock->conn->totalData -= object->len;
  spinlockRelease(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
  spinlockRelease(&sock->LOCK_SOCKET);

  if (addr && len && *len > 0) {
    sockaddr_in_linux target = {
        .sin_family = 2, .sin_port = switch_endian_16(object->remotePort)};
    memcpy(&target.sin_addr, object->ip, IPv4_BYTE_SIZE);
    int toCopyAddr = MIN(*len, sizeof(sockaddr_in_linux));
    memcpy(addr, &target, toCopyAddr);
    *len = toCopyAddr;
  }

  size_t toCopy = MIN(object->len, limit);
  memcpy(out, object->buff, toCopy);
  free(object->buff);
  free(object);

  return toCopy;
}

#define MSG_TRUNC 0x0100
size_t netSocketUdpRecvmsg(OpenFile *fd, struct msghdr_linux *msg, int flags) {
  if (!msg || !msg->msg_iov || msg->msg_iovlen == 0)
    return ERR(EINVAL);

  UdpSocket *sock = fd->dir;
  spinlockAcquire(&sock->LOCK_SOCKET);
  if (!sock->conn) {
    spinlockRelease(&sock->LOCK_SOCKET);
    return ERR(EINVAL);
  }

  udpBuffer *object = 0;
  while (true) {
    spinlockAcquire(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
    object = LinkedListSearchFirst(&sock->conn->dsBuffers);
    if (!object) {
      spinlockRelease(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
      spinlockRelease(&sock->LOCK_SOCKET);
      if (signalsPendingQuick(currentTask))
        return ERR(EINTR);
      if (fd->flags & O_NONBLOCK || flags & MSG_DONTWAIT)
        return ERR(EWOULDBLOCK);
      else
        pollIndependentAwait(fd, EPOLLIN);
    } else
      break;
  }

  assert(
      LinkedListUnregister(&sock->conn->dsBuffers, sizeof(udpBuffer), object));
  sock->conn->totalData -= object->len;
  spinlockRelease(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
  spinlockRelease(&sock->LOCK_SOCKET);

  if (msg->msg_name && msg->msg_namelen > 0) {
    sockaddr_in_linux target = {
        .sin_family = 2,
        .sin_port = switch_endian_16(object->remotePort),
    };
    memcpy(&target.sin_addr, object->ip, IPv4_BYTE_SIZE);

    size_t toCopy = MIN(msg->msg_namelen, sizeof(sockaddr_in_linux));
    memcpy(msg->msg_name, &target, toCopy);
    msg->msg_namelen = toCopy;
  }

  size_t copied = 0;
  size_t remaining = object->len;

  for (size_t i = 0; i < msg->msg_iovlen && remaining > 0; i++) {
    struct iovec *iov = &msg->msg_iov[i];
    if (!iov->iov_base || iov->iov_len == 0)
      continue;

    size_t chunk = MIN(iov->iov_len, remaining);
    memcpy(iov->iov_base, object->buff + copied, chunk);

    copied += chunk;
    remaining -= chunk;
  }

  msg->msg_flags = 0;
  if (remaining > 0)
    msg->msg_flags |= MSG_TRUNC;

  if (msg->msg_control && msg->msg_controllen > 0)
    msg->msg_controllen = 0;

  free(object->buff);
  free(object);

  return copied;
}

// maybe EPOLLERR On ICMP stuff?
// also maybe return smth when not connected to be safe w/reportKey()
int netSocketUdpInternalPoll(OpenFile *fd, int events) {
  UdpSocket *sock = fd->dir;
  int        revents = 0;
  spinlockAcquire(&sock->LOCK_SOCKET);
  if (events & EPOLLOUT)
    revents |= EPOLLOUT;
  if (events & EPOLLIN && sock->conn) {
    spinlockAcquire(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
    if (sock->conn->totalData > 0)
      revents |= EPOLLIN;
    spinlockRelease(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
  }
  spinlockRelease(&sock->LOCK_SOCKET);
  return revents;
}

size_t netSocketUdpReportKey(OpenFile *fd) {
  UdpSocket *sock = fd->dir;
  size_t     conn = (size_t)sock->conn;
  if (conn)
    return conn;
  return (size_t)sock;
}

size_t netSocketUdpGetsockname(OpenFile *fd, sockaddr_linux *addr,
                               uint32_t *len) {
  UdpSocket *sock = fd->dir;
  spinlockAcquire(&sock->LOCK_SOCKET);

  if (!sock->conn) {
    spinlockRelease(&sock->LOCK_SOCKET);
    return ERR(ENOTCONN);
  }

  assert(*len >= sizeof(sockaddr_in_linux));
  *len = sizeof(sockaddr_in_linux);

  spinlockAcquire(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
  sockaddr_in_linux *in = (sockaddr_in_linux *)addr;
  in->sin_family = 2;
  in->sin_port = switch_endian_16(sock->conn->localPort);
  memcpy(in->sin_addr, selectedNIC->ip, 4);
  memset(in->sin_zero, 0, sizeof(in->sin_zero));

  spinlockRelease(&selectedNIC->udp.LOCK_LL_UDP_CONNS);
  spinlockRelease(&sock->LOCK_SOCKET);
  return 0;
}

size_t netSocketUdpGetsockopt(OpenFile *fd, int levelLinux, int optnameLinux,
                              void *optval, uint32_t *socklen) {
  assert(false);
  return -1;
}

VfsHandlers udpSocketHandlers = {.recvfrom = netSocketUdpRecvfrom,
                                 .recvmsg = netSocketUdpRecvmsg,
                                 .internalPoll = netSocketUdpInternalPoll,
                                 .getsockname = netSocketUdpGetsockname,
                                 .getsockopts = netSocketUdpGetsockopt,
                                 .bind = netSocketUdpBind,
                                 .connect = netSocketUdpConnect,
                                 .sendto = netSocketUdpSendto,
                                 .duplicate = netSocketUdpDuplicate,
                                 .reportKey = netSocketUdpReportKey,
                                 .close = netSocketUdpClose};
