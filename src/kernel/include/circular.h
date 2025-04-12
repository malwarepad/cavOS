#include "util.h"

#ifndef CIRCULAR_H
#define CIRCULAR_H

typedef struct CircularInt {
  uint8_t *buff;
  size_t   buffSize;

  size_t readPtr;  // atomic
  size_t writePtr; // atomic

  Spinlock LOCK_READ; // NOT supposed to be in int
} CircularInt;

#define CIRC_READABLE(wr, rd, sz) ((wr - rd + sz) % sz)
#define CIRC_WRITABLE(wr, rd, sz) ((rd - wr - 1 + sz) % sz)

void   CircularIntAllocate(CircularInt *circ, size_t size);
size_t CircularIntRead(CircularInt *circ, uint8_t *buff, size_t length);
size_t CircularIntWrite(CircularInt *circ, const uint8_t *buff, size_t length);
size_t CircularIntReadPoll(CircularInt *circ);

#endif
