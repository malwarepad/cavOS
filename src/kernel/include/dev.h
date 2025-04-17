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
typedef size_t (*EventBit)(OpenFile *fd, uint64_t request, void *arg);

typedef struct DevInputEvent {
  Spinlock LOCK_USERSPACE;

  char *devname;
  char *physloc;

  size_t          timesOpened;
  CircularInt     deviceEvents;
  struct input_id inputid;

  size_t properties;

  EventBit eventBit;
} DevInputEvent;

DevInputEvent devInputEvents[MAX_EVENTS];

DevInputEvent *devInputEventSetup(char *devname);
void inputGenerateEvent(DevInputEvent *item, uint16_t type, uint16_t code,
                        int32_t value);

// dev_pty.c
#define PTY_MAX 1024
#define PTY_BUFF_SIZE 4096

typedef struct PtyPair {
  struct PtyPair *next;

  Spinlock LOCK_PTY;

  int masterFds;
  int slaveFds;

  termios  term;
  winsize  win;
  uint8_t *bufferMaster;
  uint8_t *bufferSlave;

  int ptrMaster;
  int ptrSlave;

  int  id;
  bool locked; // by default unlocked (hence 0)
} PtyPair;

VfsHandlers handlePtmx;
VfsHandlers handlePts;

void initiatePtyInterface();

size_t ptsWriteInner(PtyPair *pair, uint8_t *in, size_t limit);

#endif
