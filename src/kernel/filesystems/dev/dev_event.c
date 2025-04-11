#include <dev.h>
#include <malloc.h>
#include <string.h>
#include <syscalls.h>
#include <task.h>
#include <timer.h>
#include <util.h>

// Manages /dev/input/eventX files & binds them to their device counterparts
// Copyright (C) 2025 Panagiotis

DevInputEvent devInputEvents[MAX_EVENTS] = {0};

int lastInputEvent = 0;

void inputGenerateEvent(DevInputEvent *item, uint16_t type, uint16_t code,
                        int32_t value) {
  if (atomicRead64(&item->timesOpened) == 0)
    return;

  struct input_event event = {0};
  event.sec = timerTicks / 1000;
  event.usec = (timerTicks % 1000) * 1000;
  event.type = type;
  event.code = code;
  event.value = value;

  // todo out of sync stuff
  assert(CircularIntWrite(&item->deviceEvents, (void *)&event,
                          sizeof(struct input_event)) ==
         sizeof(struct input_event));
}

// quick utility used for getting X on /dev/input/eventX
uint64_t numAtEnd(const char *str) {
  uint64_t    num = 0;
  const char *p = str;

  while (*p)
    p++;

  const char *end = p;
  while (p > str && *(p - 1) >= '0' && *(p - 1) <= '9')
    p--; // startat

  if (p == end)
    return 0; // no digits at end

  while (p < end) // parse
    num = num * 10 + (*p++ - '0');

  return num;
}

// /dev/input/eventX userspace stuff
size_t devInputEventOpen(char *filename, int flags, int mode, OpenFile *fd,
                         char **symlinkResolve) {
  int num = numAtEnd(filename);

  DevInputEvent *event = &devInputEvents[num];
  spinlockAcquire(&event->LOCK_USERSPACE);
  event->timesOpened++;
  fd->dir = event;
  spinlockRelease(&event->LOCK_USERSPACE);
  return 0;
}

size_t devInputEventRead(OpenFile *fd, uint8_t *out, size_t limit) {
  DevInputEvent *event = fd->dir;

  while (true) {
    size_t cnt = CircularIntRead(&event->deviceEvents, out, limit);
    if (cnt > 0)
      return cnt;
    if (fd->flags & O_NONBLOCK)
      return ERR(EWOULDBLOCK);
    if (signalsPendingQuick(currentTask))
      return ERR(EINTR);
  }
}

size_t devInputEventIoctl(OpenFile *fd, uint64_t request, void *arg) {
  switch (request) {
  default:
    dbgSysFailf("non-supported ioctl!");
    return ERR(ENOTTY);
    break;
  }
}

bool devInputEventDuplicate(OpenFile *original, OpenFile *orphan) {
  orphan->dir = original->dir;

  DevInputEvent *event = original->dir;
  spinlockAcquire(&event->LOCK_USERSPACE);
  event->timesOpened++;
  spinlockRelease(&event->LOCK_USERSPACE);

  return true;
}

bool devInputEventClose(OpenFile *fd) {
  DevInputEvent *event = fd->dir;
  spinlockAcquire(&event->LOCK_USERSPACE);
  event->timesOpened--;
  spinlockRelease(&event->LOCK_USERSPACE);
  return true;
}

VfsHandlers devInputEventHandlers = {.open = devInputEventOpen,
                                     .read = devInputEventRead,
                                     .ioctl = devInputEventIoctl,
                                     .stat = fakefsFstat,
                                     .duplicate = devInputEventDuplicate,
                                     .close = devInputEventClose};

DevInputEvent *devInputEventSetup(char *devname) {
  assert(inputFakedir);
  if (lastInputEvent >= MAX_EVENTS) {
    debugf("[devfs] Reached max events!\n");
    panic();
  }

  char *name = (char *)malloc(128); // DONT free()
  snprintf(name, 128, "event%d", lastInputEvent);

  DevInputEvent *item = (DevInputEvent *)(&devInputEvents[lastInputEvent]);
  item->devname = strdup(devname);
  CircularIntAllocate(&item->deviceEvents, EVENT_BUFFER_SIZE);

  fakefsAddFile(&rootDev, inputFakedir, name, 0, S_IFREG | S_IRUSR | S_IWUSR,
                &devInputEventHandlers);
  lastInputEvent++;

  return item;
}
