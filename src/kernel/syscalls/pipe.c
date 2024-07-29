#include <console.h>
#include <fb.h>
#include <kb.h>
#include <linux.h>
#include <malloc.h>
#include <syscalls.h>
#include <task.h>

// Industrial two-way solid steel pipe()
// Copyright (C) 2024 Panagiotis

typedef struct PipeInfo {
  char buf[65536];
  int  assigned;

  int writeFds;
  int readFds;

  Spinlock LOCK;
} PipeInfo;

typedef struct PipeSpecific PipeSpecific;
struct PipeSpecific {
  bool      write;
  PipeInfo *info;
};

int pipeOpen(int *fds) {
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

  if (spec->write)
    pipe->writeFds++;
  else
    pipe->readFds++;

  return true;
}

int pipeRead(OpenFile *fd, uint8_t *out, size_t limit) {
  if (limit > 65536)
    limit = 65536;
  PipeSpecific *spec = (PipeSpecific *)fd->dir;
  PipeInfo     *pipe = spec->info;

  // check write items in this process (so we don't hang unreasonably)
  // apparently not needed :")
  // OpenFile *browse = currentTask->firstFile;
  // int       ourWriteFds = 0;
  // while (browse) {
  //   if (browse->handlers->close != pipeCloseEnd) {
  //     browse = browse->next;
  //     continue;
  //   }
  //   PipeSpecific *spec = (PipeSpecific *)browse->dir;
  //   if (spec->write)
  //     ourWriteFds++;
  //   browse = browse->next;
  // }

  // if there are no more write items, don't hang
  while (pipe->writeFds != 0 && !pipe->assigned) {
    // debugf("write{%d} curr{%d}\n", pipe->writeFds, currentTask->id);
    asm volatile("pause");
  }

  if (!pipe->assigned)
    return 0;

  spinlockAcquire(&pipe->LOCK);
  int toCopy = pipe->assigned;
  if (toCopy > limit)
    toCopy = limit;

  memcpy(out, pipe->buf, toCopy);
  pipe->assigned -= toCopy;
  memmove(pipe->buf, &pipe->buf[toCopy], 65536 - toCopy);
  spinlockRelease(&pipe->LOCK);

  return toCopy;
}

int pipeWriteInner(OpenFile *fd, uint8_t *in, size_t limit) {
  PipeSpecific *spec = (PipeSpecific *)fd->dir;
  PipeInfo     *pipe = spec->info;
  while ((pipe->assigned + limit) > 65536)
    asm volatile("pause");

  spinlockAcquire(&pipe->LOCK);
  memcpy(&pipe->buf[pipe->assigned], in, limit);
  pipe->assigned += limit;
  spinlockRelease(&pipe->LOCK);

  return limit;
}

int pipeWrite(OpenFile *fd, uint8_t *in, size_t limit) {
  int    ret = 0;
  size_t chunks = limit / 65536;
  size_t remainder = limit % 65536;
  if (chunks)
    for (size_t i = 0; i < chunks; i++) {
      int cycle = 0;
      while (cycle != 65536)
        cycle += pipeWriteInner(fd, in + i * 65536 + cycle, 65536 - cycle);
      ret += cycle;
    }

  if (remainder) {
    int cycle = 0;
    while (cycle != remainder)
      cycle +=
          pipeWriteInner(fd, in + chunks * 65536 + cycle, remainder - cycle);
    ret += cycle;
  }

  return ret;
}

bool pipeCloseEnd(OpenFile *readFd) {
  PipeSpecific *spec = (PipeSpecific *)readFd->dir;
  PipeInfo     *pipe = spec->info;

  if (spec->write)
    pipe->writeFds--;
  else
    pipe->readFds--;

  if (!pipe->readFds && !pipe->writeFds) {
    spinlockAcquire(&pipe->LOCK);
    free(pipe);
  }

  free(spec);

  return true;
}

int pipeStat(OpenFile *fd, stat *stat) {
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

int pipeBadRead() { return -EBADF; }
int pipeBadWrite() { return -EBADF; }
int pipeBadIoctl() { return -ENOTTY; }

size_t pipeBadMmap() { return -1; }

VfsHandlers pipeReadEnd = {.open = 0,
                           .close = pipeCloseEnd,
                           .duplicate = pipeDuplicate,
                           .ioctl = pipeBadIoctl,
                           .mmap = pipeBadMmap,
                           .stat = pipeStat,
                           .read = pipeRead,
                           .write = pipeBadWrite,
                           .getdents64 = 0};
VfsHandlers pipeWriteEnd = {.open = 0,
                            .close = pipeCloseEnd,
                            .duplicate = pipeDuplicate,
                            .ioctl = pipeBadIoctl,
                            .mmap = pipeBadMmap,
                            .stat = pipeStat,
                            .read = pipeBadRead,
                            .write = pipeWrite,
                            .getdents64 = 0};
