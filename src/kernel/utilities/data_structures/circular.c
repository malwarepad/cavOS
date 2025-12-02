#include <circular.h>
#include <malloc.h>
#include <util.h>

// Circular buffer implementations (mostly intended for very specific use-cases)
// Copyright (C) 2024 Panagiotis

// Circular & CircularInt have a lot in common, however it's better to keep them
// separate considering how volatile the other interface can be

// Circular: Typical circular buffer implementation that depends on locks and is
// intended to run on typical safe contexts
void CircularAllocate(Circular *circ, size_t size) {
  memset(circ, 0, sizeof(Circular));
  circ->buffSize = size;
  circ->buff = malloc(size);
}

size_t CircularRead(Circular *circ, uint8_t *buff, size_t length) {
  assert(length); // no wasting resources here

  spinlockAcquire(&circ->LOCK_CIRC);
  size_t write = circ->writePtr;
  size_t read = circ->readPtr;
  if (write == read) {
    spinlockRelease(&circ->LOCK_CIRC);
    return 0; // empty
  }

  size_t toCopy = MIN(CIRC_READABLE(write, read, circ->buffSize), length);
  size_t first = MIN(toCopy, circ->buffSize - read);
  memcpy(buff, &circ->buff[read], first);

  size_t second = toCopy - first;
  if (second)
    memcpy(buff + first, circ->buff, second);

  read = (read + toCopy) % circ->buffSize;

  assert(toCopy > 0); // only then this all makes sense
  circ->readPtr = read;
  spinlockRelease(&circ->LOCK_CIRC);

  return toCopy;
}

size_t CircularReadPoll(Circular *circ) {
  size_t ret = 0;
  spinlockAcquire(&circ->LOCK_CIRC);
  size_t write = circ->writePtr;
  size_t read = circ->readPtr;
  ret = CIRC_READABLE(write, read, circ->buffSize);
  spinlockRelease(&circ->LOCK_CIRC);
  return ret;
}

size_t CircularWritePoll(Circular *circ) {
  size_t ret = 0;
  spinlockAcquire(&circ->LOCK_CIRC);
  size_t write = circ->writePtr;
  size_t read = circ->readPtr;
  ret = CIRC_WRITABLE(write, read, circ->buffSize);
  spinlockRelease(&circ->LOCK_CIRC);
  return ret;
}

size_t CircularWrite(Circular *circ, const uint8_t *buff, size_t length) {
  assert(length); // no wasting resources here

  spinlockAcquire(&circ->LOCK_CIRC);
  size_t write = circ->writePtr;
  size_t read = circ->readPtr;
  size_t writable = CIRC_WRITABLE(write, read, circ->buffSize);
  if (length > writable) {
    spinlockRelease(&circ->LOCK_CIRC);
    return 0; // cannot do this
  }

  size_t first = MIN(length, circ->buffSize - write);
  memcpy(&circ->buff[write], buff, first);

  size_t second = length - first;
  if (second)
    memcpy(circ->buff, buff + first, second);

  write = (write + length) % circ->buffSize;

  circ->writePtr = write;
  spinlockRelease(&circ->LOCK_CIRC);
  return length;
}

void CircularFree(Circular *circ) {
  spinlockAcquire(&circ->LOCK_CIRC);
  free(circ->buff);
}

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
  for (size_t i = 0; i < toCopy; i++) {
    // todo: could optimize this with edge memcpy() operations
    buff[i] = circ->buff[read];
    read = (read + 1) % circ->buffSize;
  }

  assert(toCopy > 0); // only then this all makes sense
  atomicWrite64(&circ->readPtr, read);
  spinlockRelease(&circ->LOCK_READ);

  return toCopy;
}

size_t CircularIntReadPoll(CircularInt *circ) {
  size_t ret = 0;
  spinlockAcquire(&circ->LOCK_READ);
  size_t write = atomicRead64(&circ->writePtr);
  size_t read = atomicRead64(&circ->readPtr);
  ret = CIRC_READABLE(write, read, circ->buffSize);
  spinlockRelease(&circ->LOCK_READ);
  return ret;
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

  for (size_t i = 0; i < length; i++) {
    // todo: could optimize this with edge memcpy() operations
    circ->buff[write] = buff[i];
    write = (write + 1) % circ->buffSize;
  }

  atomicWrite64(&circ->writePtr, write);
  return length;
}
