#include "../../include/util.h"
#include "../../include/tty.h"

// Utilities used inside source code
// Copyright (C) 2023 Panagiotis

#define LONG_MASK (sizeof(unsigned long) - 1)
void memset(void *_dst, int val, size_t len) {
  unsigned char *dst = _dst;
  unsigned long *ldst;
  unsigned long  lval =
      (val & 0xFF) *
      (-1ul /
       255); // the multiplier becomes 0x0101... of the same length as long

  if (len >= 16) // optimize only if it's worth it (limit is a guess)
  {
    while ((uintptr_t)dst & LONG_MASK) {
      *dst++ = val;
      len--;
    }
    ldst = (void *)dst;
    while (len > sizeof(long)) {
      *ldst++ = lval;
      len -= sizeof(long);
    }
    dst = (void *)ldst;
  }
  while (len--)
    *dst++ = val;
}

void memory_copy(char *source, char *dest, int nbytes) {
  int i;
  for (i = 0; i < nbytes; i++) {
    *(dest + i) = *(source + i); //    dest[i] = source[i]
  }
}

void memory_set(uint8 *dest, uint8 val, uint32 len) {
  uint8 *temp = (uint8 *)dest;
  for (; len != 0; len--)
    *temp++ = val;
}

uint8 check_string(string str) { return (str[0] != 0x0); }

int memcmp(const void *aptr, const void *bptr, size_t size) {
  const unsigned char *a = (const unsigned char *)aptr;
  const unsigned char *b = (const unsigned char *)bptr;
  for (size_t i = 0; i < size; i++) {
    if (a[i] < b[i])
      return -1;
    else if (b[i] < a[i])
      return 1;
  }
  return 0;
}

static unsigned long int next = 1;

int rand(void) {
  next = next * 1103515245 + 12345;
  return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed) { next = seed; }

// int   count;
/*void *malloc(int nbytes) {
  // count += nbytes;
  // printf("\nmalloc: %d\n", count);
  char variable[nbytes];
  return &variable;
}*/
