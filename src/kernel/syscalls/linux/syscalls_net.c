#include <linux.h>
#include <socket.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>
#include <unixSocket.h>

#include <lwip/sockets.h>

// Lwip uses BSD-style sockaddr structs. We need to convert them appropriately!
uint16_t sockaddrLinuxToLwip(void *dest_addr, uint32_t addrlen) {
  sockaddr_linux  *linuxHandle = (sockaddr_linux *)dest_addr;
  struct sockaddr *handle = (struct sockaddr *)dest_addr;
  uint16_t         initialFamily = linuxHandle->sa_family;
  handle->sa_len = addrlen;
  handle->sa_family = AF_INET;
  return initialFamily;
}

void sockaddrLwipToLinux(void *dest_addr, uint16_t initialFamily) {
  sockaddr_linux *linuxHandle = (sockaddr_linux *)dest_addr;
  linuxHandle->sa_family = initialFamily;
}

#define SYSCALL_SOCKET 41
static size_t syscallSocket(int family, int type, int protocol) {
  switch (family) {
  case 1: { // AF_UNIX
    return unixSocketOpen(currentTask, type, protocol);
    break;
  }
  case 2: {
    bool cloexec = type & SOCK_CLOEXEC;
    bool nonblock = type & SOCK_NONBLOCK;
    type &= ~(SOCK_CLOEXEC | SOCK_NONBLOCK);
    int lwipFd = lwip_socket(family, type, protocol);
    if (lwipFd < 0)
      return -errno; // yes

    if (nonblock) {
      int fdflags = lwip_fcntl(lwipFd, F_GETFL, 0);
      lwip_fcntl(lwipFd, F_SETFL, fdflags | O_NONBLOCK);
    }

    int socketFd = fsUserOpen(currentTask, "/dev/stdout", O_RDWR, 0);
    if (socketFd < 0) {
      debugf("[syscalls::socket] Very bad error!\n");
      lwip_close(lwipFd);
      return -1;
    }

    OpenFile *socketNode = fsUserGetNode(currentTask, socketFd);
    if (!socketNode) {
      debugf("[syscalls::socket] Very bad error!\n");
      lwip_close(lwipFd);
      return -1;
    }

    if (cloexec)
      socketNode->closeOnExec = true;

    socketNode->handlers = &socketHandlers;

    UserSocket *userSocket = (UserSocket *)calloc(sizeof(UserSocket), 1);
    userSocket->lwipFd = lwipFd;
    userSocket->socketInstances = 1;

    socketNode->dir = userSocket;

    return socketFd;
    break;
  }
  default:
    dbgSysStubf("todo family{%d}", family);
    return ERR(ENOSYS);
    break;
  }
}

#define SYSCALL_CONNECT 42
static size_t syscallConnect(int fd, sockaddr_linux *addr, size_t len) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return ERR(EBADF);

  if (!fileNode->handlers->connect)
    return ERR(ENOTSOCK);

  return fileNode->handlers->connect(fileNode, addr, len);
}

#define SYSCALL_ACCEPT 43
static size_t syscallAccept(int fd, sockaddr_linux *addr, uint32_t *len) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return ERR(EBADF);

  if (!fileNode->handlers->accept)
    return -ENOTSOCK;
  return fileNode->handlers->accept(fileNode, addr, len);
}

#define SYSCALL_BIND 49
static size_t syscallBind(int fd, sockaddr_linux *addr, size_t len) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return ERR(EBADF);

  if (!fileNode->handlers->bind)
    return ERR(ENOTSOCK);
  return fileNode->handlers->bind(fileNode, addr, len);
}

#define SYSCALL_LISTEN 50
static size_t syscallListen(int fd, int backlog) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return ERR(EBADF);

  if (!fileNode->handlers->listen)
    return ERR(ENOTSOCK);
  return fileNode->handlers->listen(fileNode, backlog);
}

#define SYSCALL_SENDTO 44
static size_t syscallSendto(int fd, void *buff, size_t len, int flags,
                            sockaddr_linux *dest_addr, socklen_t addrlen) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return ERR(EBADF);

  if (!fileNode->handlers->sendto)
    return ERR(ENOTSOCK);
  return fileNode->handlers->sendto(fileNode, buff, len, flags, dest_addr,
                                    addrlen);
}

#define SYSCALL_RECVFROM 45
static size_t syscallRecvfrom(int fd, void *buff, size_t len, int flags,
                              sockaddr_linux *dest_addr, socklen_t *addrlen) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return ERR(EBADF);

  if (!fileNode->handlers->recvfrom)
    return ERR(ENOTSOCK);
  return fileNode->handlers->recvfrom(fileNode, buff, len, flags, dest_addr,
                                      addrlen);
}

#define SYSCALL_RECVMSG 47
static size_t syscallRecvmsg(int fd, struct msghdr *msg, int flags) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return ERR(EBADF);

  UserSocket *userSocket = (UserSocket *)fileNode->dir;

  int lwipOut = lwip_recvmsg(userSocket->lwipFd, msg, flags);
  sockaddrLwipToLinux(msg->msg_name, AF_INET);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

void syscallsRegNet() {
  // a
  registerSyscall(SYSCALL_SOCKET, syscallSocket);
  registerSyscall(SYSCALL_CONNECT, syscallConnect);
  registerSyscall(SYSCALL_ACCEPT, syscallAccept);
  registerSyscall(SYSCALL_BIND, syscallBind);
  registerSyscall(SYSCALL_SENDTO, syscallSendto);
  registerSyscall(SYSCALL_RECVFROM, syscallRecvfrom);
  registerSyscall(SYSCALL_RECVMSG, syscallRecvmsg);
  registerSyscall(SYSCALL_LISTEN, syscallListen);
}
