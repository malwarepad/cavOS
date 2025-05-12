#include <console.h>
#include <fb.h>
#include <kb.h>
#include <linux.h>
#include <syscalls.h>
#include <task.h>

#include <malloc.h>

// Manages /dev/std* files and the like
// Copyright (C) 2024 Panagiotis

size_t readHandler(OpenFile *fd, uint8_t *in, size_t limit) {
  // console fb
  // while (kbIsOccupied()) {
  // } done in kbTaskRead()

  // assign new buffer
  uint8_t *kernelBuff = malloc(limit);

  // start reading
  kbTaskRead(currentTask->id, (char *)kernelBuff, limit, true);
  asm volatile("sti"); // leave this task/execution (awaiting return)
  while (currentTask->state == TASK_STATE_WAITING_INPUT) {
    handControl();
  }
  if (currentTask->term.c_lflag & ICANON)
    printf("\n"); // you technically pressed enter, didn't you?

  // finalise
  uint32_t fr = currentTask->tmpRecV;
  memcpy(in, kernelBuff, fr);
  if (currentTask->term.c_lflag & ICANON && fr < limit)
    in[fr++] = '\n';
  // only add newline if we can!

  return fr;
}

size_t writeHandler(OpenFile *fd, uint8_t *out, size_t limit) {
  // console fb
  for (int i = 0; i < limit; i++) {
    // #if DEBUG_SYSCALLS_EXTRA
    //     serial_send(COM1, out[i]);
    // #endif
    printfch(out[i]);
  }
  return limit;
}

size_t ioctlHandler(OpenFile *fd, uint64_t request, void *arg) {
  switch (request) {
  case 0x5413: {
    winsize *win = (winsize *)arg;
    win->ws_row = fb.height / TTY_CHARACTER_HEIGHT;
    win->ws_col = fb.width / TTY_CHARACTER_WIDTH;

    win->ws_xpixel = fb.width;
    win->ws_ypixel = fb.height;
    return 0;
    break;
  }
  case TCGETS: {
    memcpy(arg, &currentTask->term, sizeof(termios));
    // debugf("got %d %d\n", currentTask->term.c_lflag & ICANON,
    //        currentTask->term.c_lflag & ECHO);
    return 0;
    break;
  }
  case TCSETS:
  case TCSETSW:   // this drains(?), idek man
  case TCSETSF: { // idek anymore man
    memcpy(&currentTask->term, arg, sizeof(termios));
    // debugf("setting %d %d\n", currentTask->term.c_lflag & ICANON,
    //        currentTask->term.c_lflag & ECHO);
    return 0;
    break;
  }
  case 0x540f: // TIOCGPGRP
  {
    int *pid = (int *)arg;
    *pid = currentTask->id;
    return 0;
    break;
  }
  default:
    return -1;
    break;
  }
}

size_t mmapHandler(size_t addr, size_t length, int prot, int flags,
                   OpenFile *fd, size_t pgoffset) {
  debugf("[io::mmap] FATAL! Tried to mmap on stdio!\n");
  return -1;
}

size_t statHandler(OpenFile *fd, stat *target) {
  target->st_dev = 420;
  target->st_ino = rand(); // todo!
  target->st_mode = S_IFCHR | S_IRUSR | S_IWUSR;
  target->st_nlink = 1;
  target->st_uid = 0;
  target->st_gid = 0;
  target->st_rdev = 34830;
  target->st_blksize = 0x1000;
  target->st_size = 0;
  target->st_blocks = DivRoundUp(target->st_size, 512);
  target->st_atime = 69;
  target->st_mtime = 69;
  target->st_ctime = 69;

  return 0;
}

// if this doesn't give you enough cues as to why you NEED to avoid the kernel
// shell at all costs then idk what does. the reason this exists btw is for
// bash's readline to not freak out over pselect6()
bool ioSwitch = false;
int  internalPollHandler(OpenFile *fd, int events) {
  int revents = 0;
  if (events & EPOLLIN && ioSwitch)
    revents |= EPOLLIN;
  if (events & EPOLLOUT)
    revents |= EPOLLOUT;
  ioSwitch = !ioSwitch;
  return revents;
}

VfsHandlers stdio = {.open = 0,
                     .close = 0,
                     .read = readHandler,
                     .write = writeHandler,
                     .ioctl = ioctlHandler,
                     .mmap = mmapHandler,
                     .stat = statHandler,
                     .internalPoll = internalPollHandler,
                     .duplicate = 0,
                     .getdents64 = 0};
