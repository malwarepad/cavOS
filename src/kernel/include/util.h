#ifndef UTIL_H
#define UTIL_H

#include "types.h"

#define DIV_ROUND_CLOSEST(n, d)                                                \
  ((((n) < 0) == ((d) < 0)) ? (((n) + (d) / 2) / (d)) : (((n) - (d) / 2) / (d)))

#define DivRoundUp(number, divisor) ((number + divisor - 1) / divisor)
#define inrand(minimum_number, max_number)                                     \
  (rand() % (max_number + 1 - minimum_number) + minimum_number)

#define SPLIT_64_HIGHER(value) ((value) >> 32)
#define SPLIT_64_LOWER(value) ((value) & 0xFFFFFFFF)

void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size);
void *memmove(void *dstptr, const void *srcptr, size_t size);
void  memset(void *_dst, int val, size_t len);
int   memcmp(const void *aptr, const void *bptr, size_t size);
int   rand(void);
void  srand(unsigned int seed);
void  hexDump(const char *desc, const void *addr, const int len, int perLine);

#endif
