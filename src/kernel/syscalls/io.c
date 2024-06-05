#include <console.h>
#include <fb.h>
#include <kb.h>
#include <syscalls.h>
#include <task.h>

// Manages /dev/std* files and the like
// Copyright (C) 2024 Panagiotis

int readHandler(OpenFile *fd, uint8_t *in, size_t limit) {
  // console fb
  // while (kbIsOccupied()) {
  // } done in kbTaskRead()

  // start reading
  kbTaskRead(currentTask->id, (char *)in, limit, true);
  asm volatile("sti"); // leave this task/execution (awaiting return)
  while (currentTask->state == TASK_STATE_WAITING_INPUT) {
  }
  if (currentTask->term.c_lflag & ICANON)
    printf("\n"); // you technically pressed enter, didn't you?

  // finalise
  uint32_t fr = currentTask->tmpRecV;
  if (currentTask->term.c_lflag & ICANON && fr < limit)
    in[fr++] = '\n';
  // only add newline if we can!

  return fr;
}

int writeHandler(OpenFile *fd, uint8_t *out, size_t limit) {
  // console fb
  for (int i = 0; i < limit; i++) {
#if DEBUG_SYSCALLS
    serial_send(COM1, out[i]);
#endif
    printfch(out[i]);
  }
  return limit;
}

int ioctlHandler(OpenFile *fd, uint64_t request, void *arg) {
  switch (request) {
  case 0x5413: {
    winsize *win = (winsize *)arg;
    win->ws_row = framebufferHeight / TTY_CHARACTER_HEIGHT;
    win->ws_col = framebufferWidth / TTY_CHARACTER_WIDTH;

    win->ws_xpixel = framebufferWidth;
    win->ws_ypixel = framebufferHeight;
    return 0;
    break;
  }
  case TCGETS: {
    memcpy(arg, &currentTask->term, sizeof(termios));
    debugf("got %d %d\n", currentTask->term.c_lflag & ICANON,
           currentTask->term.c_lflag & ECHO);
    return 0;
    break;
  }
  case TCSETS: {
    memcpy(&currentTask->term, arg, sizeof(termios));
    debugf("setting %d %d\n", currentTask->term.c_lflag & ICANON,
           currentTask->term.c_lflag & ECHO);
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

SpecialHandlers stdio = {.read = readHandler,
                         .write = writeHandler,
                         .ioctl = ioctlHandler,
                         .mmap = mmapHandler};
