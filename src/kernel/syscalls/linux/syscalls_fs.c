#include <syscalls.h>
#include <task.h>
#include <util.h>

#define SYSCALL_READ 0
static int syscallRead(int fd, char *str, uint32_t count) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::read] file{%d} str{%x} count{%d}\n", fd, str, count);
#endif

  OpenFile *browse = fsUserGetNode(fd);
  if (!browse) {
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls::read] FAIL! Couldn't find file! fd{%d}\n", fd);
#endif
    return -1;
  }
  uint32_t read = fsRead(browse, (uint8_t *)str, count);
#if DEBUG_SYSCALLS_ARGS
  debugf("\nread = %d\n", read);
#endif
  return read;
}

#define SYSCALL_WRITE 1
static int syscallWrite(int fd, char *str, uint32_t count) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::write] file{%d} str{%x} count{%d}\n", fd, str, count);
#endif
  OpenFile *browse = fsUserGetNode(fd);
  if (!browse) {
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls::write] FAIL! Couldn't find file! fd{%d}\n", fd);
#endif
    return -1;
  }

  uint32_t writtenBytes = fsWrite(browse, (uint8_t *)str, count);
  return writtenBytes;
}

#define SYSCALL_OPEN 2
static int syscallOpen(char *filename, int flags, uint16_t mode) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::open] filename{%s} flags{%d} mode{%d}\n", filename, flags,
         mode);
#endif
  if (!filename) {
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls::open] FAIL! No filename given... yeah.\n");
#endif
    return -1;
  }

  int ret = fsUserOpen(filename, flags, mode);
  if (ret < 0) {
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls::open] FAIL! Couldn't open file! filename{%s}\n",
           filename);
#endif
  }

  return ret;
}

#define SYSCALL_CLOSE 3
static int syscallClose(int fd) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::close] fd{%d}\n", fd);
#endif

  return fsUserClose(fd);
}

#define SYSCALL_STAT 4
static int syscallStat(char *filename, stat *statbuf) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::stat] filename{%s} buff{%lx}\n", filename, statbuf);
#endif
  bool ret = fsStat(filename, statbuf, 0);
  if (!ret) {
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls::stat] FAIL! Couldn't stat() file! filename{%s}\n",
           filename);
#endif
    return -1;
  }

  return 0;
}

#define SYSCALL_FSTAT 5
static int syscallFstat(int fd, stat *statbuf) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::fstat] fd{%d} buff{%lx}\n", fd, statbuf);
#endif
  OpenFile *file = fsUserGetNode(fd);
  if (!file)
    return -1;

  bool ret = fsStat(file->safeFilename, statbuf, 0);
  if (!ret) {
#if DEBUG_SYSCALLS_FAILS
    debugf("[syscalls::fstat] FAIL! Couldn't stat() file! fd{%d}\n", fd);
#endif
    return -1;
  }

  return 0;
}

#define SYSCALL_LSEEK 8
static int syscallLseek(uint32_t file, int offset, int whence) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::seek] file{%d} offset{%d} whence{%d}\n", file, offset,
         whence);
#endif
  // todo!
  if (file == 0 || file == 1)
    return -1;
  return fsUserSeek(file, offset, whence);
}

#define SYSCALL_IOCTL 16
static int syscallIoctl(int fd, unsigned long request, void *arg) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::ioctl] fd{%d} req{%lx} arg{%lx}\n", fd, request, arg);
#endif
  OpenFile *browse = fsUserGetNode(fd);
  if (!browse || browse->mountPoint != MOUNT_POINT_SPECIAL) {
#if DEBUG_SYSCALLS_FAILS
    debugf(
        "[syscalls::ioctl] FAIL! Tried to manipulate non-special file! fd{%d} "
        "req{%lx} arg{%lx}\n",
        fd, request, arg);
#endif
    return -1;
  }

  SpecialFile *special = (SpecialFile *)browse->dir;
  if (!special)
    return -1;

  int ret = special->ioctlHandler(browse, request, arg);

#if DEBUG_SYSCALLS_STUB
  if (ret < 0)
    debugf("[syscalls::ioctl] UNIMPLEMENTED! fd{%d} req{%lx} arg{%lx}\n", fd,
           request, arg);
#endif

  return ret;
}

#define SYSCALL_WRITEV 20
static int syscallWriteV(uint32_t fd, iovec *iov, uint32_t ioVcnt) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::writev] fd{%d} iov{%lx} iovcnt{%d}\n", fd, iov, ioVcnt);
#endif

  int cnt = 0;
  for (int i = 0; i < ioVcnt; i++) {
    iovec *curr = (iovec *)((size_t)iov + i * sizeof(iovec));

#if DEBUG_SYSCALLS
    debugf(
        "[syscalls::writev(%d)] fd{%d} iov_base{%x} iov_len{%x} iovcnt{%d}\n",
        i, fd, curr->iov_base, curr->iov_len, ioVcnt);
#endif
    int singleCnt = syscallWrite(fd, curr->iov_base, curr->iov_len);
    if (singleCnt == -1)
      return cnt;

    cnt += singleCnt;
  }

  return cnt;
}

#define SYSCALL_READV 19
static int syscallReadV(uint32_t fd, iovec *iov, uint32_t ioVcnt) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::readv] fd{%d} iov{%lx} iovcnt{%d}\n", fd, iov, ioVcnt);
#endif

  int cnt = 0;
  for (int i = 0; i < ioVcnt; i++) {
    iovec *curr = (iovec *)((size_t)iov + i * sizeof(iovec));

#if DEBUG_SYSCALLS
    debugf("[syscalls::readv(%d)] fd{%d} iov_base{%x} iov_len{%x} iovcnt{%d}\n",
           i, fd, curr->iov_base, curr->iov_len, ioVcnt);
#endif
    int singleCnt = syscallRead(fd, curr->iov_base, curr->iov_len);
    if (singleCnt == -1)
      return cnt;

    cnt += singleCnt;
  }

  return cnt;
}

#define SYSCALL_DUP 32
static int syscallDup(uint32_t fd) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::dup] fd{%d}\n", fd);
#endif
  OpenFile *file = fsUserGetNode(fd);
  if (!file)
    return -1;

  OpenFile *new = fsUserDuplicateNode(currentTask, file);

  return new ? new->id : -1;
}

#define SYSCALL_DUP2 33
static int syscallDup2(uint32_t oldFd, uint32_t newFd) {
#if DEBUG_SYSCALLS_ARGS
  debugf("[syscalls::dup2] old{%d} new{%d}\n", oldFd, newFd);
#endif

  // todo: treat 0, 1 and the like FDs like actual files
  debugf("[syscalls::dup2] UNIMPLEMENTED! old{%d} new{%d}\n", oldFd, newFd);
  return -1;
}

void syscallRegFs() {
  registerSyscall(SYSCALL_WRITE, syscallWrite);
  registerSyscall(SYSCALL_READ, syscallRead);
  registerSyscall(SYSCALL_OPEN, syscallOpen);
  registerSyscall(SYSCALL_CLOSE, syscallClose);
  registerSyscall(SYSCALL_LSEEK, syscallLseek);
  registerSyscall(SYSCALL_STAT, syscallStat);
  registerSyscall(SYSCALL_FSTAT, syscallFstat);

  registerSyscall(SYSCALL_IOCTL, syscallIoctl);
  registerSyscall(SYSCALL_READV, syscallReadV);
  registerSyscall(SYSCALL_WRITEV, syscallWriteV);
  registerSyscall(SYSCALL_DUP2, syscallDup2);
  registerSyscall(SYSCALL_DUP, syscallDup);
}