#include <fat32.h>
#include <linked_list.h>
#include <linux.h>
#include <malloc.h>
#include <string.h>
#include <syscalls.h>
#include <task.h>
#include <util.h>

// Manages all systemcalls related to filesystem operations
// Copyright (C) 2024 Panagiotis

#define SYSCALL_READ 0
static size_t syscallRead(int fd, char *str, uint32_t count) {
  if (!count)
    return 0;
  OpenFile *browse = fsUserGetNode(currentTask, fd);
  if (!browse)
    return ERR(EBADF);
  return fsRead(browse, (uint8_t *)str, count);
}

#define SYSCALL_WRITE 1
static size_t syscallWrite(int fd, char *str, uint32_t count) {
  if (!count)
    return 0;
  OpenFile *browse = fsUserGetNode(currentTask, fd);
  if (!browse)
    return ERR(EBADF);
  return fsWrite(browse, (uint8_t *)str, count);
}

#define SYSCALL_OPEN 2
static size_t syscallOpen(char *filename, int flags, int mode) {
  dbgSysExtraf("filename{%s}", filename);
  if (!filename)
    return ERR(EFAULT);
  return fsUserOpen(currentTask, filename, flags, mode);
}

#define SYSCALL_CLOSE 3
static size_t syscallClose(int fd) { return fsUserClose(currentTask, fd); }

#define SYSCALL_STAT 4
static size_t syscallStat(char *filename, stat *statbuf) {
  dbgSysExtraf("filename{%s}", filename);
  bool ret = fsStatByFilename(currentTask, filename, statbuf);
  return (ret ? 0 : ERR(ENOENT));
}

#define SYSCALL_FSTAT 5
static size_t syscallFstat(int fd, stat *statbuf) {
  OpenFile *file = fsUserGetNode(currentTask, fd);
  if (!file)
    return ERR(EBADF);

  bool ret = fsStat(file, statbuf);
  return (ret ? 0 : ERR(ENOENT));
}

#define SYSCALL_LSTAT 6
static size_t syscallLstat(char *filename, stat *statbuf) {
  bool ret = fsLstatByFilename(currentTask, filename, statbuf);
  return (ret ? 0 : ERR(ENOENT));
}

#include <timer.h>
#define SYSCALL_POLL 7
static size_t syscallPoll(struct pollfd *fds, int nfds, int timeout) {
  int ret = 0;
  for (int i = 0; i < nfds; i++) {
    uint64_t start = timerTicks;
    fds[i].revents = 0; // as a start
    if (fds[i].fd == -1)
      continue; // ignore -1 fds

    OpenFile *browse = fsUserGetNode(currentTask, fds[i].fd);
    if (!browse || !browse->handlers->poll)
      continue;

    if (browse->handlers->poll(browse, &fds[i], timeout))
      ret++;

    // ack time
    if (timeout <= (timerTicks - start))
      timeout = 0;
    else
      timeout -= timerTicks - start;
  }
  return ret;
}

#define SYSCALL_LSEEK 8
static size_t syscallLseek(uint32_t file, int offset, int whence) {
  return fsUserSeek(currentTask, file, offset, whence);
}

#define SYSCALL_IOCTL 16
static size_t syscallIoctl(int fd, unsigned long request, void *arg) {
  OpenFile *browse = fsUserGetNode(currentTask, fd);
  if (!browse)
    return ERR(EBADF);

  if (!browse->handlers->ioctl) {
    dbgSysFailf("non-special file");
    return ERR(ENOTTY);
  }

  return browse->handlers->ioctl(browse, request, arg);
}

#define SYSCALL_PREAD64 17
static size_t syscallPread64(uint64_t fd, char *buff, size_t count,
                             size_t pos) {
  size_t seekOp = syscallLseek(fd, pos, SEEK_SET);
  if (RET_IS_ERR(seekOp))
    return seekOp;
  size_t readOp = syscallRead(fd, buff, count);
  return readOp;
}

#define SYSCALL_WRITEV 20
static size_t syscallWriteV(uint32_t fd, iovec *iov, uint32_t ioVcnt) {
  size_t cnt = 0;
  for (int i = 0; i < ioVcnt; i++) {
    iovec *curr = (iovec *)((size_t)iov + i * sizeof(iovec));

    size_t singleCnt = syscallWrite(fd, curr->iov_base, curr->iov_len);
    if (RET_IS_ERR(singleCnt))
      return singleCnt;

    cnt += singleCnt;
  }

  return cnt;
}

#define SYSCALL_READV 19
static size_t syscallReadV(uint32_t fd, iovec *iov, uint32_t ioVcnt) {
  size_t cnt = 0;
  for (int i = 0; i < ioVcnt; i++) {
    iovec *curr = (iovec *)((size_t)iov + i * sizeof(iovec));

    size_t singleCnt = syscallRead(fd, curr->iov_base, curr->iov_len);
    if (RET_IS_ERR(singleCnt))
      return singleCnt;

    cnt += singleCnt;
  }

  return cnt;
}

#define SYSCALL_ACCESS 21
static size_t syscallAccess(char *filename, int mode) {
  struct stat buf;
  return syscallStat(filename, &buf);
}

#define SYSCALL_DUP 32
static size_t syscallDup(uint32_t fd) {
  OpenFile *file = fsUserGetNode(currentTask, fd);
  if (!file)
    return ERR(EBADF);

  OpenFile *new = fsUserDuplicateNode(currentTask, file);
  new->closeOnExec = 0; // does not persist

  return new ? new->id : -1;
}

#define SYSCALL_DUP2 33
static size_t syscallDup2(uint32_t oldFd, uint32_t newFd) {
  OpenFile *realFile = fsUserGetNode(currentTask, oldFd);
  if (!realFile)
    return ERR(EBADF);

  if (oldFd == newFd)
    return newFd;

  if (fsUserGetNode(currentTask, newFd))
    fsUserClose(currentTask, newFd);

  // OpenFile    *realFile = currentTask->firstFile;
  OpenFile *targetFile = fsUserDuplicateNodeUnsafe(realFile);
  targetFile->closeOnExec = 0; // does not persist

  LinkedListPushFrontUnsafe((void **)(&currentTask->firstFile), targetFile);

  targetFile->id = newFd;
  return newFd;
}

#define SYSCALL_FCNTL 72
static size_t syscallFcntl(int fd, int cmd, uint64_t arg) {
  OpenFile *file = fsUserGetNode(currentTask, fd);
  if (!file)
    return ERR(EBADF);
  if (file->handlers->fcntl)
    file->handlers->fcntl(file, cmd, arg);
  switch (cmd) {
  case F_GETFD:
    return file->closeOnExec;
    break;
  case F_SETFD:
    file->closeOnExec = !file->closeOnExec;
    return 0;
    break;
  case F_DUPFD:
    (void)arg; // todo: not ignore arg
    return syscallDup(fd);
    break;
  case F_GETFL:
    return file->flags;
    break;
  case F_SETFL: {
    int validFlags = O_APPEND | FASYNC | O_DIRECT | O_NOATIME | O_NONBLOCK;
    file->flags &= ~validFlags;
    file->flags |= arg & validFlags;
    return 0;
    break;
  }
  case F_DUPFD_CLOEXEC: {
    size_t targ = syscallDup(fd);
    if (RET_IS_ERR(targ))
      return targ;

    size_t res = syscallFcntl(targ, F_SETFD, FD_CLOEXEC);
    if (RET_IS_ERR(res))
      return res;

    return targ;
  }
  default:
    dbgSysStubf("cmd{%d} not implemented", cmd);
    return -1;
    break;
  }
}

#define SYSCALL_MKDIR 83
static size_t syscallMkdir(char *path, uint32_t mode) {
  dbgSysExtraf("path{%s}", path);
  mode &= ~(currentTask->umask);
  return fsMkdir(currentTask, path, mode);
}

#define SYSCALL_UNLINK 87
static size_t syscallUnlink(char *path) {
  return fsUnlink(currentTask, path, false);
}

#define SYSCALL_READLINK 89
static size_t syscallReadlink(char *path, char *buf, int size) {
  dbgSysExtraf("path{%s}", path);
  return fsReadlink(currentTask, path, buf, size);
}

#define SYSCALL_UMASK 95
static size_t syscallUmask(uint32_t mask) {
  int old = currentTask->umask;
  currentTask->umask = mask & 0777;
  return old;
}

#define SYSCALL_GETDENTS64 217
static size_t syscallGetdents64(unsigned int fd, struct linux_dirent64 *dirp,
                                unsigned int count) {
  OpenFile *browse = fsUserGetNode(currentTask, fd);
  if (!browse)
    return ERR(EBADF);
  if (!browse->handlers->getdents64)
    return ERR(ENOTDIR);
  return browse->handlers->getdents64(browse, dirp, count);
}

#define FD_SETSIZE 1024

typedef unsigned long fd_mask;

typedef struct {
  unsigned long fds_bits[FD_SETSIZE / 8 / sizeof(long)];
} fd_set;

#define SYSCALL_PSELECT6 270
static size_t syscallPselect6(int nfds, fd_set *readfds, fd_set *writefds,
                              fd_set *exceptfds, struct timespec *timeout,
                              void *smthsignalthing) {
  if (timeout && !timeout->tv_nsec && !timeout->tv_sec)
    return 0;

  int amnt = 0;
  int bits_per_long = sizeof(unsigned long) * 8;
  if (readfds)
    for (int fd = 0; fd < nfds; fd++) {
      int index = fd / bits_per_long;
      int bit = fd % bits_per_long;
      if (readfds->fds_bits[index] & (1UL << bit)) {
        // todo: uhm.. poll?
        amnt++;
      }
    }
  if (writefds)
    for (int fd = 0; fd < nfds; fd++) {
      int index = fd / bits_per_long;
      int bit = fd % bits_per_long;
      if (writefds->fds_bits[index] & (1UL << bit)) {
        // todo: uhm.. poll?
        amnt++;
      }
    }
  if (exceptfds)
    for (int fd = 0; fd < nfds; fd++) {
      int index = fd / bits_per_long;
      int bit = fd % bits_per_long;
      if (exceptfds->fds_bits[index] & (1UL << bit)) {
        // todo: uhm.. poll?
        amnt++;
      }
    }

  // todo: timelimit (for when I actually poll)

  return amnt;
}

#define SYSCALL_SELECT 23
static size_t syscallSelect(int nfds, fd_set *readfds, fd_set *writefds,
                            fd_set *exceptfds, struct timeval *timeout) {
  if (timeout) {
    struct timespec timeoutformat = {.tv_sec = timeout->tv_sec,
                                     .tv_nsec = timeout->tv_usec * 1000};
    return syscallPselect6(nfds, readfds, writefds, exceptfds, &timeoutformat,
                           0);
  }
  return syscallPselect6(nfds, readfds, writefds, exceptfds, 0, 0);
}

char *atResolvePathname(int dirfd, char *pathname) {
  assert(pathname[0] != '\0'); // by fd, should've checked before running!
  if (pathname[0] == '/') {    // by absolute pathname
    return pathname;
  } else if (pathname[0] != '/') {
    if (dirfd == AT_FDCWD) { // relative to cwd
      return pathname;
    } else { // relative to dirfd, resolve accordingly
      OpenFile *fd = fsUserGetNode(currentTask, dirfd);
      if (!fd)
        return (char *)ERR(EBADF);
      if (!fd->dirname)
        return (char *)ERR(ENOTDIR);

      int prefixLen = strlength(fd->mountPoint->prefix);
      int rootDirLen = strlength(fd->dirname);
      int pathnameLen = strlength(pathname) + 1;

      char *out = malloc(prefixLen + rootDirLen + 1 + pathnameLen);

      memcpy(out, fd->mountPoint->prefix, prefixLen);
      memcpy(&out[prefixLen], fd->dirname, rootDirLen);
      out[prefixLen + rootDirLen] = '/'; // better be safe than sorry
      memcpy(&out[prefixLen + rootDirLen + 1], pathname, pathnameLen);
      return out;
    }
  }

  assert(false); // will never be reached
  return 0;
}

void atResolvePathnameCleanup(char *pathname, char *resolved) {
  if ((size_t)pathname != (size_t)resolved)
    free(resolved);
}

#define SYSCALL_OPENAT 257
static size_t syscallOpenat(int dirfd, char *pathname, int flags, int mode) {
  if (pathname[0] == '\0') { // by fd
    return dirfd;
  }

  char *resolved = atResolvePathname(dirfd, pathname);
  if (RET_IS_ERR((size_t)resolved))
    return (size_t)resolved;

  size_t ret = syscallOpen(resolved, flags, mode);
  atResolvePathnameCleanup(pathname, resolved);
  return ret;
}

#define SYSCALL_MKDIRAT 258
static size_t syscallMkdirAt(int dirfd, char *pathname, int mode) {
  if (pathname[0] == '\0') { // by fd
    return ERR(ENOENT);
  }

  char *resolved = atResolvePathname(dirfd, pathname);
  if (RET_IS_ERR((size_t)resolved))
    return (size_t)resolved;

  size_t ret = syscallMkdir(resolved, mode);
  atResolvePathnameCleanup(pathname, resolved);
  return ret;
}

#define SYSCALL_NEWFSTATAT 262
static size_t syscallNewfstatat(int dirfd, char *pathname, struct stat *statbuf,
                                int flag) {
  if (pathname[0] == '\0')
    return syscallFstat(dirfd, statbuf);

  char *resolved = atResolvePathname(dirfd, pathname);
  if (RET_IS_ERR((size_t)resolved))
    return (size_t)resolved;

  size_t ret = flag & AT_SYMLINK_NOFOLLOW ? syscallLstat(resolved, statbuf)
                                          : syscallStat(resolved, statbuf);
  atResolvePathnameCleanup(pathname, resolved);
  return ret;
}

#define SYSCALL_UNLINKAT 263
static size_t syscallUnlinkat(int dirfd, char *pathname, int mode) {
  bool directory = mode & 0x200;
  if (pathname[0] == '\0') { // by fd
    dbgSysFailf("unsupported!");
    return ERR(ENOSYS);
  }

  char *resolved = atResolvePathname(dirfd, pathname);
  if (RET_IS_ERR((size_t)resolved))
    return (size_t)resolved;

  size_t ret = fsUnlink(currentTask, resolved, directory);
  atResolvePathnameCleanup(pathname, resolved);
  return ret;
}

#define SYSCALL_FACCESSAT 269
static size_t syscallFaccessat(int dirfd, char *pathname, int mode) {
  if (pathname[0] == '\0') { // by fd
    return 0;
  }

  char *resolved = atResolvePathname(dirfd, pathname);
  if (RET_IS_ERR((size_t)resolved))
    return (size_t)resolved;

  size_t ret = syscallAccess(resolved, mode);
  atResolvePathnameCleanup(pathname, resolved);
  return ret;
}

#define SYSCALL_STATX 332
static size_t syscallStatx(int dirfd, char *pathname, int flags, uint32_t mask,
                           struct statx *buff) {
  struct stat simple = {0};
  size_t      statRet = syscallNewfstatat(dirfd, pathname, &simple, flags);
  if (RET_IS_ERR(statRet))
    return statRet;

  memset(buff, 0, sizeof(struct statx));

  buff->stx_mask = mask;
  buff->stx_blksize = simple.st_blksize;
  buff->stx_attributes = 0; // naw
  buff->stx_nlink = simple.st_nlink;
  buff->stx_uid = simple.st_uid;
  buff->stx_gid = simple.st_gid;
  buff->stx_mode = simple.st_mode;
  buff->stx_ino = simple.st_ino;
  buff->stx_size = simple.st_size;
  buff->stx_blocks = simple.st_blocks;
  buff->stx_attributes_mask = 0; // naw

  buff->stx_atime.tv_sec = simple.st_atime;
  buff->stx_atime.tv_nsec = simple.st_atimensec;

  // well, it's a creation time but whatever
  buff->stx_btime.tv_sec = simple.st_ctime;
  buff->stx_btime.tv_nsec = simple.st_ctimensec;

  buff->stx_ctime.tv_sec = simple.st_ctime;
  buff->stx_ctime.tv_nsec = simple.st_ctimensec;

  buff->stx_mtime.tv_sec = simple.st_mtime;
  buff->stx_mtime.tv_nsec = simple.st_mtimensec;

  // todo: special devices

  return 0;
}

// #define SYSCALL_FACCESSAT2 439
// static size_t syscallFaccessat2(int dirfd, char *pathname, int mode, int
// flags)
// {
//   return -1;
// }

void syscallRegFs() {
  registerSyscall(SYSCALL_WRITE, syscallWrite);
  registerSyscall(SYSCALL_READ, syscallRead);
  registerSyscall(SYSCALL_OPEN, syscallOpen);
  registerSyscall(SYSCALL_OPENAT, syscallOpenat);
  registerSyscall(SYSCALL_POLL, syscallPoll);
  registerSyscall(SYSCALL_MKDIRAT, syscallMkdirAt);
  registerSyscall(SYSCALL_CLOSE, syscallClose);
  registerSyscall(SYSCALL_LSEEK, syscallLseek);
  registerSyscall(SYSCALL_STAT, syscallStat);
  registerSyscall(SYSCALL_FSTAT, syscallFstat);
  registerSyscall(SYSCALL_LSTAT, syscallLstat);
  registerSyscall(SYSCALL_MKDIR, syscallMkdir);
  registerSyscall(SYSCALL_UMASK, syscallUmask);
  registerSyscall(SYSCALL_PREAD64, syscallPread64);
  registerSyscall(SYSCALL_UNLINK, syscallUnlink);

  registerSyscall(SYSCALL_IOCTL, syscallIoctl);
  registerSyscall(SYSCALL_READV, syscallReadV);
  registerSyscall(SYSCALL_WRITEV, syscallWriteV);
  registerSyscall(SYSCALL_DUP2, syscallDup2);
  registerSyscall(SYSCALL_DUP, syscallDup);
  registerSyscall(SYSCALL_ACCESS, syscallAccess);
  registerSyscall(SYSCALL_FACCESSAT, syscallFaccessat);
  registerSyscall(SYSCALL_GETDENTS64, syscallGetdents64);
  registerSyscall(SYSCALL_PSELECT6, syscallPselect6);
  registerSyscall(SYSCALL_SELECT, syscallSelect);
  registerSyscall(SYSCALL_FCNTL, syscallFcntl);
  registerSyscall(SYSCALL_STATX, syscallStatx);
  registerSyscall(SYSCALL_READLINK, syscallReadlink);
  registerSyscall(SYSCALL_NEWFSTATAT, syscallNewfstatat);
  registerSyscall(SYSCALL_UNLINKAT, syscallUnlinkat);
  // registerSyscall(SYSCALL_FACCESSAT2, syscallFaccessat2);
}