#include <console.h>
#include <fb.h>
#include <kb.h>
#include <linux.h>
#include <malloc.h>
#include <poll.h>
#include <syscalls.h>
#include <task.h>

// Industrial two-way solid steel pipe()
// Copyright (C) 2024 Panagiotis

#define PIPE_BUFF 65536
typedef struct PipeInfo {
  char buf[PIPE_BUFF];
  int  assigned;

  int writeFds;
  int readFds;

  Spinlock LOCK;
  Blocking blockingRead;
  Blocking blockingWrite;
} PipeInfo;

typedef struct PipeSpecific PipeSpecific;
struct PipeSpecific {
  bool      write;
  PipeInfo *info;
};

size_t pipeOpen(int *fds) {
  int readFd = fsUserOpen(currentTask, "/dev/stdout", O_RDONLY, 0);
  int writeFd = fsUserOpen(currentTask, "/dev/stdout", O_WRONLY, 0);

  if (readFd < 0 || writeFd < 0)
    goto bad_error;

  OpenFile *read = fsUserGetNode(currentTask, readFd);
  OpenFile *write = fsUserGetNode(currentTask, writeFd);

  if (!read || !write)
    goto bad_error;

  read->handlers = &pipeReadEnd;
  write->handlers = &pipeWriteEnd;

  PipeInfo *info = (PipeInfo *)malloc(sizeof(PipeInfo));
  memset(info, 0, sizeof(PipeInfo));
  info->readFds = 1;
  info->writeFds = 1;

  PipeSpecific *readSpec = (PipeSpecific *)malloc(sizeof(PipeSpecific));
  readSpec->write = false;
  readSpec->info = info;

  PipeSpecific *writeSpec = (PipeSpecific *)malloc(sizeof(PipeSpecific));
  writeSpec->write = true;
  writeSpec->info = info;

  read->dir = readSpec;
  write->dir = writeSpec;

  fds[0] = readFd;
  fds[1] = writeFd;

  dbgSysExtraf("fds{%d, %d}", fds[0], fds[1]);

  return 0;

bad_error:
  debugf("[pipe] Very bad error!\n");
  return -1;
}

bool pipeDuplicate(OpenFile *original, OpenFile *orphan) {
  orphan->dir = malloc(sizeof(PipeSpecific));
  memcpy(orphan->dir, original->dir, sizeof(PipeSpecific));

  PipeSpecific *spec = (PipeSpecific *)original->dir;
  PipeInfo     *pipe = spec->info;

  spinlockAcquire(&pipe->LOCK);
  if (spec->write)
    pipe->writeFds++;
  else
    pipe->readFds++;
  spinlockRelease(&pipe->LOCK);

  return true;
}

size_t pipeRead(OpenFile *fd, uint8_t *out, size_t limit) {
  if (limit > PIPE_BUFF)
    limit = PIPE_BUFF;
  PipeSpecific *spec = (PipeSpecific *)fd->dir;
  PipeInfo     *pipe = spec->info;

  // if there are no more write items, don't hang
  while (true) {
    spinlockAcquire(&pipe->LOCK);
    if (pipe->writeFds == 0 || pipe->assigned > 0)
      break;
    if (fd->flags & O_NONBLOCK) {
      spinlockRelease(&pipe->LOCK);
      return ERR(EWOULDBLOCK);
    }
    taskBlock(&pipe->blockingRead, currentTask, &pipe->LOCK, true);
    handControl();
  }

  if (!pipe->assigned) {
    spinlockRelease(&pipe->LOCK);
    return 0;
  }

  // we already have a spinlock!
  int toCopy = pipe->assigned;
  if (toCopy > limit)
    toCopy = limit;

  memcpy(out, pipe->buf, toCopy);
  pipe->assigned -= toCopy;
  memmove(pipe->buf, &pipe->buf[toCopy], PIPE_BUFF - toCopy);
  taskUnblock(&pipe->blockingWrite);
  spinlockRelease(&pipe->LOCK);
  pollInstanceRing((size_t)pipe, EPOLLOUT);

  return toCopy;
}

size_t pipeWriteInner(OpenFile *fd, uint8_t *in, size_t limit) {
  PipeSpecific *spec = (PipeSpecific *)fd->dir;
  PipeInfo     *pipe = spec->info;
  while (true) {
    spinlockAcquire(&pipe->LOCK);
    if ((pipe->assigned + limit) <= PIPE_BUFF)
      break;
    if (!pipe->readFds) { // we are notified, no worries
      spinlockRelease(&pipe->LOCK);
      atomicBitmapSet(&currentTask->sigPendingList, SIGPIPE);
      return ERR(EPIPE);
    }
    if (fd->flags & O_NONBLOCK) {
      spinlockRelease(&pipe->LOCK);
      return ERR(EWOULDBLOCK);
    }
    taskBlock(&pipe->blockingWrite, currentTask, &pipe->LOCK, true);
    handControl();
  }

  // we already have a spinlock!
  memcpy(&pipe->buf[pipe->assigned], in, limit);
  pipe->assigned += limit;
  taskUnblock(&pipe->blockingRead);
  spinlockRelease(&pipe->LOCK);
  pollInstanceRing((size_t)pipe, EPOLLIN);

  return limit;
}

size_t pipeWrite(OpenFile *fd, uint8_t *in, size_t limit) {
  int    ret = 0;
  size_t chunks = limit / PIPE_BUFF;
  size_t remainder = limit % PIPE_BUFF;
  if (chunks)
    for (size_t i = 0; i < chunks; i++) {
      int cycle = 0;
      while (cycle != PIPE_BUFF) {
        size_t innerRet =
            pipeWriteInner(fd, in + i * PIPE_BUFF + cycle, PIPE_BUFF - cycle);
        if (RET_IS_ERR(innerRet))
          return innerRet; // cycle || ret ignored since only EPIPE & EAGAIN
        cycle += innerRet;
      }
      ret += cycle;
    }

  if (remainder) {
    int cycle = 0;
    while (cycle != remainder) {
      size_t innerRet = pipeWriteInner(fd, in + chunks * PIPE_BUFF + cycle,
                                       remainder - cycle);
      if (RET_IS_ERR(innerRet))
        return innerRet; // cycle || ret ignored since only EPIPE & EAGAIN
      cycle += innerRet;
    }
    ret += cycle;
  }

  return ret;
}

int pipeInternalPoll(OpenFile *fd, int events) {
  PipeSpecific *spec = (PipeSpecific *)fd->dir;
  PipeInfo     *pipe = spec->info;

  int out = 0;

  spinlockAcquire(&pipe->LOCK);
  if (events & EPOLLIN) {
    if (!pipe->writeFds)
      out |= EPOLLHUP;
    else if (pipe->assigned > 0)
      out |= EPOLLIN;
  }

  if (events & EPOLLOUT) {
    if (!pipe->readFds)
      out |= EPOLLERR;
    else if (pipe->assigned < PIPE_BUFF)
      out |= EPOLLOUT;
  }
  spinlockRelease(&pipe->LOCK);
  return out;
}

size_t pipeReportKey(OpenFile *fd) {
  PipeSpecific *spec = (PipeSpecific *)fd->dir;
  PipeInfo     *pipe = spec->info;
  return (size_t)pipe;
}

bool pipeCloseEnd(OpenFile *readFd) {
  PipeSpecific *spec = (PipeSpecific *)readFd->dir;
  PipeInfo     *pipe = spec->info;

  spinlockAcquire(&pipe->LOCK);
  if (spec->write)
    pipe->writeFds--;
  else
    pipe->readFds--;

  int pollWith = 0;
  if (!pipe->writeFds) { // edge case
    taskUnblock(&pipe->blockingRead);
    pollWith |= EPOLLHUP;
  }
  if (!pipe->readFds) { // edge case (more aggressive)
    taskUnblock(&pipe->blockingWrite);
    pollWith |= EPOLLERR;
  }
  spinlockRelease(&pipe->LOCK);

  if (pollWith)
    pollInstanceRing((size_t)pipe, pollWith);

  if (!pipe->readFds && !pipe->writeFds) {
    spinlockAcquire(&pipe->LOCK);
    free(pipe);
  }

  free(spec);

  return true;
}

size_t pipeStat(OpenFile *fd, stat *stat) {
  stat->st_mode = 0x1180;
  stat->st_dev = 70;
  stat->st_ino = rand(); // todo!
  stat->st_mode = S_IFCHR | S_IRUSR | S_IWUSR;
  stat->st_nlink = 1;
  stat->st_uid = 0;
  stat->st_gid = 0;
  stat->st_rdev = 0;
  stat->st_blksize = 0x1000;
  stat->st_size = 0;
  stat->st_blocks = DivRoundUp(stat->st_size, 512);
  stat->st_atime = 69;
  stat->st_mtime = 69;
  stat->st_ctime = 69;
  return 0;
}

size_t pipeBadRead() { return ERR(EBADF); }
size_t pipeBadWrite() { return ERR(EBADF); }
size_t pipeBadIoctl() { return ERR(ENOTTY); }

size_t pipeBadMmap() { return -1; }

VfsHandlers pipeReadEnd = {.open = 0,
                           .close = pipeCloseEnd,
                           .duplicate = pipeDuplicate,
                           .ioctl = pipeBadIoctl,
                           .mmap = pipeBadMmap,
                           .stat = pipeStat,
                           .read = pipeRead,
                           .write = pipeBadWrite,
                           .internalPoll = pipeInternalPoll,
                           .reportKey = pipeReportKey,
                           .getdents64 = 0};
VfsHandlers pipeWriteEnd = {.open = 0,
                            .close = pipeCloseEnd,
                            .duplicate = pipeDuplicate,
                            .ioctl = pipeBadIoctl,
                            .mmap = pipeBadMmap,
                            .stat = pipeStat,
                            .read = pipeBadRead,
                            .write = pipeWrite,
                            .internalPoll = pipeInternalPoll,
                            .reportKey = pipeReportKey,
                            .getdents64 = 0};
