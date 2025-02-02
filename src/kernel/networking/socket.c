#include <linux.h>
#include <socket.h>
#include <system.h>
#include <task.h>
#include <timer.h>

#include <lwip/sockets.h>

size_t socketRead(OpenFile *fd, uint8_t *out, size_t limit) {
  UserSocket *userSocket = (UserSocket *)fd->dir;

  int lwipOut = lwip_read(userSocket->lwipFd, out, limit);
  if (lwipOut < 0)
    return -errno;
  return lwipOut;
}

size_t socketWrite(OpenFile *fd, uint8_t *in, size_t limit) {
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