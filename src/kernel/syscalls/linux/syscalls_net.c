#include <linux.h>
#include <syscalls.h>
#include <system.h>
#include <task.h>
#include <timer.h>

#include <lwip/sockets.h>

typedef struct UserSocket {
  Spinlock LOCK_INSTANCE;

  int socketInstances;
  int lwipFd;
} UserSocket;

typedef struct {
  uint16_t sa_family;
  char     sa_data[];
} sockaddr_linux;

// Lwip uses BSD-style sockaddr structs. We need to convert them appropriately!
force_inline uint16_t sockaddrLinuxToLwip(struct sockaddr *dest_addr,
                                          uint32_t         addrlen) {
  sockaddr_linux *linuxHandle = (sockaddr_linux *)dest_addr;
  uint16_t        initialFamily = linuxHandle->sa_family;
  dest_addr->sa_len = addrlen;
  dest_addr->sa_family = AF_INET;
  return initialFamily;
}

force_inline void sockaddrLwipToLinux(struct sockaddr *dest_addr,
                                      uint16_t         initialFamily) {
  sockaddr_linux *linuxHandle = (sockaddr_linux *)dest_addr;
  linuxHandle->sa_family = initialFamily;
}

int socketRead(OpenFile *fd, uint8_t *out, size_t limit) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  int lwipOut = lwip_read(userSocket->lwipFd, out, limit);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

int socketWrite(OpenFile *fd, uint8_t *in, size_t limit) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  int lwipOut = lwip_write(userSocket->lwipFd, in, limit);
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

void socketFcntl(OpenFile *fd, int cmd, uint64_t arg) {
  UserSocket *userSocket = (UserSocket *)fd->dir;
  if (cmd == F_GETFL || cmd == F_SETFL)
    lwip_fcntl(userSocket->lwipFd, cmd, arg);
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

bool socketPoll(OpenFile *fd, struct pollfd *pollfd, int timeout) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  int initial = pollfd->fd;
  pollfd->fd = userSocket->lwipFd;
  int      ret = 0;
  uint64_t timerStart = timerTicks;
  do {
    ret = lwip_poll(pollfd, 1, 0);
    if (ret == 1)
      break;
  } while (timeout && timerTicks < (timerStart + timeout));
  pollfd->fd = initial;

  return ret == 1;
}

VfsHandlers socketHandlers = {.read = socketRead,
                              .write = socketWrite,
                              .fcntl = socketFcntl,
                              .poll = socketPoll,
                              .duplicate = socketDuplicate,
                              .close = socketClose};

#define SYSCALL_SOCKET 41
static int syscallSocket(int family, int type, int protocol) {
  switch (family) {
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
#if DEBUG_SYSCALLS_STUB
    debugf("[syscalls::socket] Tried to create a socket of family{%d}\n",
           family);
#endif
    return -ENOSYS;
    break;
  }
}

#define SYSCALL_CONNECT 42
static int syscallConnect(int fd, struct sockaddr *addr, size_t len) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return -EBADF;

  UserSocket *userSocket = (UserSocket *)fileNode->dir;

  uint16_t initialFamily = sockaddrLinuxToLwip(addr, len);
  int      lwipOut = lwip_connect(userSocket->lwipFd, addr, len);
  sockaddrLwipToLinux(addr, initialFamily);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

#define SYSCALL_BIND 49
static int syscallBind(int fd, struct sockaddr *addr, size_t len) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return -EBADF;

  addr->sa_family = AF_INET;

  UserSocket *userSocket = (UserSocket *)fileNode->dir;

  int lwipOut = lwip_bind(userSocket->lwipFd, addr, len);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

#define SYSCALL_SENDTO 44
static int syscallSendto(int fd, void *buff, size_t len, int flags,
                         struct sockaddr *dest_addr, socklen_t addrlen) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return -EBADF;

  UserSocket *userSocket = (UserSocket *)fileNode->dir;

  uint16_t initialFamily = sockaddrLinuxToLwip(dest_addr, addrlen);
  int      lwipOut =
      lwip_sendto(userSocket->lwipFd, buff, len, flags, dest_addr, addrlen);
  sockaddrLwipToLinux(dest_addr, initialFamily);

  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

#define SYSCALL_RECVMSG 47
static int syscallRecvmsg(int fd, struct msghdr *msg, int flags) {
  OpenFile *fileNode = fsUserGetNode(currentTask, fd);
  if (!fileNode)
    return -EBADF;

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
  registerSyscall(SYSCALL_BIND, syscallBind);
  registerSyscall(SYSCALL_SENDTO, syscallSendto);
  registerSyscall(SYSCALL_RECVMSG, syscallRecvmsg);
}
