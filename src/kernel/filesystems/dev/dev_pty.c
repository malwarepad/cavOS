#include <console.h>
#include <dev.h>
#include <fb.h>
#include <linked_list.h>
#include <malloc.h>
#include <string.h>
#include <syscalls.h>
#include <task.h>
#include <timer.h>
#include <util.h>

// Manages /dev/pts/X & /dev/ptmx files and the general pty interface
// Copyright (C) 2025 Panagiotis

// Note that this file is mostly a stub as it's not my focus yet

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
  pair->win.ws_row = 24;
  pair->win.ws_col = 80; // some sane defaults
  fd->dir = pair;
  spinlockRelease(&LOCK_PTY_GLOBAL);
  return 0;
}

// todo: control + d stuff
size_t ptmxDataAvail(PtyPair *pair) {
  return pair->ptrMaster; // won't matter here
}

size_t ptmxRead(OpenFile *fd, uint8_t *out, size_t limit) {
  PtyPair *pair = fd->dir;
  while (true) {
    spinlockAcquire(&pair->LOCK_PTY);
    if (ptmxDataAvail(pair) > 0)
      break;
    if (!pair->slaveFds) {
      spinlockRelease(&pair->LOCK_PTY);
      return 0;
    }
    if (fd->flags & O_NONBLOCK) {
      spinlockRelease(&pair->LOCK_PTY);
      return ERR(EWOULDBLOCK);
    }
    spinlockRelease(&pair->LOCK_PTY);
    handControl();
  }

  size_t toCopy = MIN(limit, ptmxDataAvail(pair));
  memcpy(out, pair->bufferMaster, toCopy);
  memmove(pair->bufferMaster, &pair->bufferMaster[toCopy],
          PTY_BUFF_SIZE - toCopy);
  pair->ptrMaster -= toCopy;

  spinlockRelease(&pair->LOCK_PTY);
  return toCopy;
}

size_t ptmxWrite(OpenFile *fd, uint8_t *in, size_t limit) {
  assert(limit <= PTY_BUFF_SIZE); // todo
  PtyPair *pair = fd->dir;
  while (true) {
    spinlockAcquire(&pair->LOCK_PTY);
    if (!pair->slaveFds) {
      spinlockRelease(&pair->LOCK_PTY);
      return 0;
    }
    if ((pair->ptrSlave + limit) < PTY_BUFF_SIZE)
      break;
    if (fd->flags & O_NONBLOCK) {
      spinlockRelease(&pair->LOCK_PTY);
      return ERR(EWOULDBLOCK);
    }
    spinlockRelease(&pair->LOCK_PTY);
    handControl();
  }

  // we already have a lock in our hands
  memcpy(&pair->bufferSlave[pair->ptrSlave], in, limit);
  if (pair->term.c_iflag & ICRNL)
    for (size_t i = 0; i < limit; i++) {
      if (pair->bufferSlave[pair->ptrSlave + i] == '\r')
        pair->bufferSlave[pair->ptrSlave + i] = '\n';
    }
  pair->ptrSlave += limit;
  if (pair->term.c_lflag & ICANON && pair->term.c_lflag & ECHO) {
    assert(ptsWriteInner(pair, &pair->bufferSlave[pair->ptrSlave - limit],
                         limit) == limit);
  }
  // hexDump("fr", in, limit, 32, debugf);

  spinlockRelease(&pair->LOCK_PTY);
  return limit;
}

size_t ptmxIoctl(OpenFile *fd, uint64_t request, void *arg) {
  PtyPair *pair = fd->dir;
  size_t   ret = 0; // todo ERR(ENOTTY)
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
  switch (request) {
  case TIOCGWINSZ: {
    memcpy(arg, &pair->win, sizeof(winsize));
    ret = 0;
    break;
  }
  case TIOCSWINSZ: {
    memcpy(&pair->win, arg, sizeof(winsize));
    ret = 0;
    break;
  }
  }
  spinlockRelease(&pair->LOCK_PTY);

  return ret;
}

int ptmxInternalPoll(OpenFile *fd, int events) {
  PtyPair *pair = fd->dir;
  int      revents = 0;

  spinlockAcquire(&pair->LOCK_PTY);
  if (ptmxDataAvail(pair) > 0 && events & EPOLLIN)
    revents |= EPOLLIN;
  if (pair->ptrSlave < PTY_BUFF_SIZE && events & EPOLLOUT)
    revents |= EPOLLOUT;
  spinlockRelease(&pair->LOCK_PTY);

  return revents;
}

VfsHandlers handlePtmx = {.open = ptmxOpen,
                          .read = ptmxRead,
                          .write = ptmxWrite,
                          .internalPoll = ptmxInternalPoll,
                          .ioctl = ptmxIoctl};

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

  if (browse->locked) {
    spinlockRelease(&browse->LOCK_PTY);
    return ERR(EIO);
  }

  fd->dir = browse;
  browse->slaveFds++;
  spinlockRelease(&browse->LOCK_PTY);
  return 0;
}

size_t ptsDataAvail(PtyPair *pair) {
  bool canonical = pair->term.c_lflag & ICANON;
  if (!canonical)
    return pair->ptrSlave; // flush whatever we can

  // now we're on canonical mode
  for (size_t i = 0; i < pair->ptrSlave; i++) {
    if (pair->bufferSlave[i] == '\n' ||
        pair->bufferSlave[i] == pair->term.c_cc[VEOF] ||
        pair->bufferSlave[i] == pair->term.c_cc[VEOL] ||
        pair->bufferSlave[i] == pair->term.c_cc[VEOL2])
      return i + 1; // +1 for len
  }

  return 0; // nothing found
}

size_t ptsRead(OpenFile *fd, uint8_t *out, size_t limit) {
  PtyPair *pair = fd->dir;
  while (true) {
    spinlockAcquire(&pair->LOCK_PTY);
    if (ptsDataAvail(pair) > 0)
      break;
    if (!pair->masterFds) {
      spinlockRelease(&pair->LOCK_PTY);
      return 0;
    }
    if (fd->flags & O_NONBLOCK) {
      spinlockRelease(&pair->LOCK_PTY);
      return ERR(EWOULDBLOCK);
    }
    spinlockRelease(&pair->LOCK_PTY);
    handControl();
  }

  size_t toCopy = MIN(limit, ptsDataAvail(pair));
  memcpy(out, pair->bufferSlave, toCopy);
  memmove(pair->bufferSlave, &pair->bufferSlave[toCopy],
          PTY_BUFF_SIZE - toCopy);
  pair->ptrSlave -= toCopy;

  spinlockRelease(&pair->LOCK_PTY);
  return toCopy;
}

size_t ptsWriteInner(PtyPair *pair, uint8_t *in, size_t limit) {
  size_t written = 0;
  bool   doTranslate =
      (pair->term.c_oflag & OPOST) && (pair->term.c_oflag & ONLCR);
  for (size_t i = 0; i < limit; ++i) {
    uint8_t ch = in[i];
    if (doTranslate && ch == '\n') {
      if ((pair->ptrMaster + 2) >= PTY_BUFF_SIZE)
        break;
      pair->bufferMaster[pair->ptrMaster++] = '\r';
      pair->bufferMaster[pair->ptrMaster++] = '\n';
      written++;
    } else {
      if ((pair->ptrMaster + 1) >= PTY_BUFF_SIZE)
        break;
      pair->bufferMaster[pair->ptrMaster++] = ch;
      written++;
    }
  }
  return written;
}

size_t ptsWrite(OpenFile *fd, uint8_t *in, size_t limit) {
  assert(limit <= PTY_BUFF_SIZE); // todo
  PtyPair *pair = fd->dir;
  while (true) {
    spinlockAcquire(&pair->LOCK_PTY);
    if (!pair->masterFds) {
      spinlockRelease(&pair->LOCK_PTY);
      return 0;
    }
    if ((pair->ptrMaster + limit) < PTY_BUFF_SIZE)
      break;
    if (fd->flags & O_NONBLOCK) {
      spinlockRelease(&pair->LOCK_PTY);
      return ERR(EWOULDBLOCK);
    }
    spinlockRelease(&pair->LOCK_PTY);
    handControl();
  }

  // we already have a lock in our hands
  size_t written = ptsWriteInner(pair, in, limit);

  spinlockRelease(&pair->LOCK_PTY);
  return written;
}

size_t ptsIoctl(OpenFile *fd, uint64_t request, void *arg) {
  PtyPair *pair = fd->dir;
  size_t   ret = ERR(ENOTTY);

  spinlockAcquire(&pair->LOCK_PTY);
  switch (request) {
  case TIOCGWINSZ: {
    memcpy(arg, &pair->win, sizeof(winsize));
    ret = 0;
    break;
  }
  case TIOCSWINSZ: {
    memcpy(&pair->win, arg, sizeof(winsize));
    ret = 0;
    break;
  }
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

int ptsInternalPoll(OpenFile *fd, int events) {
  PtyPair *pair = fd->dir;
  int      revents = 0;

  spinlockAcquire(&pair->LOCK_PTY);
  if (ptsDataAvail(pair) > 0 && events & EPOLLIN)
    revents |= EPOLLIN;
  if (pair->ptrMaster < PTY_BUFF_SIZE && events & EPOLLOUT)
    revents |= EPOLLOUT;
  spinlockRelease(&pair->LOCK_PTY);

  return revents;
}

VfsHandlers handlePts = {.open = ptsOpen,
                         .read = ptsRead,
                         .write = ptsWrite,
                         .internalPoll = ptsInternalPoll,
                         .ioctl = ptsIoctl,
                         .stat = fakefsFstat};
