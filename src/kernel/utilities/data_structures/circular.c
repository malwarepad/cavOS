#include <circular.h>
#include <malloc.h>
#include <util.h>

// Circular buffer implementations (mostly intended for very specific use-cases)
// Copyright (C) 2024 Panagiotis

// CircularInt: lock-less (mostly) supposed to be written to from interrupt
// contexts. there *is* a lock however for reading, to ensure only one thread
// can do so.
void CircularIntAllocate(CircularInt *circ, size_t size) {
  memset(circ, 0, sizeof(CircularInt));
  circ->buffSize = size;
  circ->buff = malloc(size);
}

size_t CircularIntRead(CircularInt *circ, uint8_t *buff, size_t length) {
  assert(length); // no wasting resources here

  spinlockAcquire(&circ->LOCK_READ);
  size_t write = atomicRead64(&circ->writePtr);
  size_t read = atomicRead64(&circ->readPtr);
  if (write == read) {
    spinlockRelease(&circ->LOCK_READ);
    return 0; // empty
  }

  size_t toCopy = MIN(CIRC_READABLE(write, read, circ->buffSize), length);
  for (int i = 0; i < toCopy; i++) {
    // todo: could optimize this with edge memcpy() operations
    buff[i] = circ->buff[read];
    read = (read + 1) % circ->buffSize;
  }

  assert(toCopy > 0); // only then this all makes sense
  atomicWrite64(&circ->readPtr, read);
  spinlockRelease(&circ->LOCK_READ);

  return toCopy;
}

// maybe here we could use a separate lock when smp stuff is established
// (SPECIFIC for interrupts, since no handControl() can be used)
size_t CircularIntWrite(CircularInt *circ, const uint8_t *buff, size_t length) {
  assert(!checkInterrupts()); // should only be ran from an int context
  assert(length);             // no wasting resources here

  size_t write = atomicRead64(&circ->writePtr);
  size_t read = atomicRead64(&circ->readPtr);
  size_t writable = CIRC_WRITABLE(write, read, circ->buffSize);
  if (length > writable) {
    return 0; // cannot do this
  }

  for (int i = 0; i < length; i++) {
    // todo: could optimize this with edge memcpy() operations
    circ->buff[write] = buff[i];
    write = (write + 1) % circ->buffSize;
  }

  atomicWrite64(&circ->writePtr, write);
  return length;
}
