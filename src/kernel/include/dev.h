#include "circular.h"
#include "fakefs.h"
#include "linux.h"
#include "types.h"
#include "vfs.h"

#ifndef DEV_H
#define DEV_H

// dev_controller.c
FakefsFile *inputFakedir;
Fakefs      rootDev;

bool devMount(MountPoint *mount);

// dev_event.c
#define MAX_EVENTS 8
#define EVENT_BUFFER_SIZE 16384

typedef struct DevInputEvent {
  Spinlock LOCK_USERSPACE;

  char       *devname;
  size_t      timesOpened;
  CircularInt deviceEvents;
} DevInputEvent;

DevInputEvent devInputEvents[MAX_EVENTS];

DevInputEvent *devInputEventSetup(char *devname);
void inputGenerateEvent(DevInputEvent *item, uint16_t type, uint16_t code,
                        int32_t value);

#endif
