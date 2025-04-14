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

void ioctlWrite(void *target, size_t val, int size) {
  int toCopy = MIN(sizeof(size_t), size);
  memcpy(target, &val, toCopy);
}

size_t devInputEventIoctl(OpenFile *fd, uint64_t request, void *arg) {
  DevInputEvent *event = fd->dir;
  size_t         type = _IOC_TYPE(request);
  size_t         dir = _IOC_DIR(request);
  size_t         number = _IOC_NR(request);
  size_t         size = _IOC_SIZE(request);

  (void)type;
  (void)dir;

  dbgSysExtraf("number{0x%lx} size{%ld}", number, size);

  size_t ret = ERR(ENOTTY);

  assert(event->eventBit);
  if (number >= 0x20 && number < (0x20 + EV_CNT)) {
    // we are in EVIOCGBIT(event: 0x20 - x) territory, beware
    return event->eventBit(fd, request, arg);
  } else if (number >= 0x40 && number < (0x40 + ABS_CNT)) {
    // we are in EVIOCGABS(event: 0x40 - x) territory, beware
    return event->eventBit(fd, request, arg);
  }

  if (request == 0x540b) // TCFLSH, idk why don't ask me!
    return ERR(EINVAL);

  switch (number) {
  case 0x01: // EVIOCGVERSION idk, stolen from vmware
    *((int *)arg) = 0x10001;
    ret = 0;
    break;
  case 0x02: // EVIOCGID
    memcpy(arg, &event->inputid, sizeof(struct input_id));
    ret = 0;
    break;
  case 0x06: { // EVIOCGNAME(len)
    int toCopy = MIN(size, strlength(event->devname) + 1);
    memcpy(arg, event->devname, toCopy);
    ret = toCopy;
    break;
  }
  case 0x07: { // EVIOCGPHYS(len)
    int toCopy = MIN(size, strlength(event->physloc) + 1);
    memcpy(arg, event->physloc, toCopy);
    ret = toCopy;
    break;
  }
  case 0x08: // EVIOCGUNIQ()
    ret = ERR(ENOENT);
    break;
  case 0x09: // EVIOCGPROP()
    ioctlWrite(arg, event->properties, size);
    ret = size;
    break;
  case 0x18: // EVIOCGKEY()
    ret = event->eventBit(fd, request, arg);
    break;
  case 0x19: // EVIOCGLED()
    ret = event->eventBit(fd, request, arg);
    break;
  case 0x1b: // EVIOCGSW()
    ret = event->eventBit(fd, request, arg);
    break;
  default:
    dbgSysFailf("non-supported ioctl! %lx", number);
    ret = ERR(ENOTTY);
    break;
  }

  return ret;
}

int devInputInternalPoll(OpenFile *fd, int events) {
  DevInputEvent *event = fd->dir;
  size_t         cnt = CircularIntReadPoll(&event->deviceEvents);
  if (cnt > 0 && events & EPOLLIN)
    return EPOLLIN;
  return 0;
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
                                     .internalPoll = devInputInternalPoll,
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

  char *physloc = "serio1";
  item->physloc = strdup(physloc);

  fakefsAddFile(&rootDev, inputFakedir, name, 0, S_IFREG | S_IRUSR | S_IWUSR,
                &devInputEventHandlers);
  lastInputEvent++;

  return item;
}
