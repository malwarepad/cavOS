#include <dev.h>
#include <linked_list.h>
#include <malloc.h>
#include <string.h>
#include <syscalls.h>
#include <task.h>
#include <timer.h>
#include <util.h>

// Manages /dev/pts/X & /dev/ptmx files and the general pty interface
// Copyright (C) 2025 Panagiotis

// todo duplicate, close, cleanup. on both ends
// this file is mostly a stub as it's not my focus yet

uint8_t *ptyBitmap = 0;
Spinlock LOCK_PTY_GLOBAL = {0};

PtyPair *firstPtyPair = 0;

int ptyBitmapDecide() {
  int ret = -1;
  spinlockAcquire(&LOCK_PTY_GLOBAL);
  for (int i = 0; i < PTY_MAX; i++) {
    if (!bitmapGenericGet(ptyBitmap, i)) {
      bitmapGenericSet(ptyBitmap, i, true);
      ret = i;
      break;
    }
  }
  spinlockRelease(&LOCK_PTY_GLOBAL);

  assert(ret != -1); // todo: proper errors
  return ret;
}

void ptyBitmapRemove(int index) {
  spinlockAcquire(&LOCK_PTY_GLOBAL);
  assert(bitmapGenericGet(ptyBitmap, index));
  bitmapGenericSet(ptyBitmap, index, false);
  spinlockRelease(&LOCK_PTY_GLOBAL);
}

void ptyTermiosDefaults(struct termios *term) {
  term->c_iflag = ICRNL | IXON | BRKINT | ISTRIP | INPCK;
  term->c_oflag = OPOST | ONLCR;
  term->c_cflag = B38400 | CS8 | CREAD | HUPCL;
  term->c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK;

  term->c_cc[VINTR] = 3;    // Ctrl-C
  term->c_cc[VQUIT] = 28;   // Ctrl-backslash
  term->c_cc[VERASE] = 127; // DEL
  term->c_cc[VKILL] = 21;   // Ctrl-U
  term->c_cc[VEOF] = 4;     // Ctrl-D
  term->c_cc[VTIME] = 0;
  term->c_cc[VMIN] = 1;
  term->c_cc[VSTART] = 17; // Ctrl-Q
  term->c_cc[VSTOP] = 19;  // Ctrl-S
  term->c_cc[VSUSP] = 26;  // Ctrl-Z
}

void initiatePtyInterface() {
  ptyBitmap = calloc(PTY_MAX / 8, 1);
  // a
}

size_t ptmxOpen(char *filename, int flags, int mode, OpenFile *fd,
                char **symlinkResolve) {
  int id = ptyBitmapDecide(); // here to avoid double locks
  spinlockAcquire(&LOCK_PTY_GLOBAL);
  PtyPair *pair = LinkedListAllocate((void **)&firstPtyPair, sizeof(PtyPair));
  pair->masterFds = 1;
  pair->id = id;
  pair->bufferMaster = malloc(PTY_BUFF_SIZE);
  pair->bufferSlave = malloc(PTY_BUFF_SIZE);
  ptyTermiosDefaults(&pair->term);
  fd->dir = pair;
  spinlockRelease(&LOCK_PTY_GLOBAL);
  return 0;
}

size_t ptmxRead(OpenFile *fd, uint8_t *out, size_t limit) {
  PtyPair *pair = fd->dir;
  while (true) {
    spinlockAcquire(&pair->LOCK_PTY);
    if (pair->slaveFds == 0 || pair->ptrMaster > 0)
      break;
    if (fd->flags & O_NONBLOCK) {
      spinlockRelease(&pair->LOCK_PTY);
      return ERR(EWOULDBLOCK);
    }
    spinlockRelease(&pair->LOCK_PTY);
    handControl();
  }

  spinlockRelease(&pair->LOCK_PTY);
  return ERR(ENOSYS); // todo all of this, think through canonical, etc
}

size_t ptmxIoctl(OpenFile *fd, uint64_t request, void *arg) {
  PtyPair *pair = fd->dir;
  size_t   ret = ERR(ENOTTY);
  size_t   number = _IOC_NR(request);

  spinlockAcquire(&pair->LOCK_PTY);
  switch (number) {
  case 0x31: { // TIOCSPTLCK
    int lock = *((int *)arg);
    dbgSysExtraf("lock{%d}", lock);
    if (lock == 0)
      pair->locked = false;
    else
      pair->locked = true;
    ret = 0;
    break;
  }
  case 0x30: // TIOCGPTN
    *((int *)arg) = pair->id;
    ret = 0;
    break;
  }
  spinlockRelease(&pair->LOCK_PTY);

  return ret;
}
VfsHandlers handlePtmx = {.open = ptmxOpen, .ioctl = ptmxIoctl};

size_t ptsOpen(char *filename, int flags, int mode, OpenFile *fd,
               char **symlinkResolve) {
  int length = strlength(filename);
  int slashes = 0; // /pts/X has to be EXACTLY 2
  for (int i = 0; i < length; i++) {
    if (filename[i] == '/')
      slashes++;
  }
  if (slashes != 2)
    return ERR(ENOENT);

  uint64_t id = numAtEnd(filename);
  spinlockAcquire(&LOCK_PTY_GLOBAL);
  PtyPair *browse = firstPtyPair;
  while (browse) {
    spinlockAcquire(&browse->LOCK_PTY);
    if (browse->id == id)
      break;
    spinlockRelease(&browse->LOCK_PTY);
    browse = browse->next;
  }
  spinlockRelease(&LOCK_PTY_GLOBAL);

  if (!browse)
    return ERR(ENOENT);

  fd->dir = browse;
  browse->slaveFds++;
  spinlockRelease(&browse->LOCK_PTY);
  return 0;
}

size_t ptsIoctl(OpenFile *fd, uint64_t request, void *arg) {
  PtyPair *pair = fd->dir;
  size_t   ret = ERR(ENOTTY);

  spinlockAcquire(&pair->LOCK_PTY);
  switch (request) {
  case TIOCSCTTY: { // todo
    ret = 0;
    break;
  }
  case TCGETS: {
    memcpy(arg, &pair->term, sizeof(termios));
    ret = 0;
    break;
  }
  case TCSETS:
  case TCSETSW:   // this drains(?), idek man
  case TCSETSF: { // idek anymore man
    memcpy(&pair->term, arg, sizeof(termios));
    ret = 0;
    break;
  }
  case 0x540f: // TIOCGPGRP
    // todo: controlling stuff. also verify understanding on io.c
    ret = 0;
    break;
  }
  spinlockRelease(&pair->LOCK_PTY);

  return ret;
}

VfsHandlers handlePts = {
    .open = ptsOpen, .ioctl = ptsIoctl, .stat = fakefsFstat};
