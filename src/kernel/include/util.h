#ifndef UTIL_H
#define UTIL_H

#include "types.h"

#define DIV_ROUND_CLOSEST(n, d)                                                \
  ((((n) < 0) == ((d) < 0)) ? (((n) + (d) / 2) / (d)) : (((n) - (d) / 2) / (d)))

#define DivRoundUp(number, divisor) ((number + divisor - 1) / divisor)
#define inrand(minimum_number, max_number)                                     \
  (rand() % (max_number + 1 - minimum_number) + minimum_number)

#define COMBINE_64(higher, lower)                                              \
  (((uint64_t)(higher) << 32) | (uint64_t)(lower))
#define SPLIT_64_HIGHER(value) ((value) >> 32)
#define SPLIT_64_LOWER(value) ((value) & 0xFFFFFFFF)

#define SPLIT_32_HIGHER(value) ((value) >> 16)
#define SPLIT_32_LOWER(value) ((value) & 0xFFFF)

#define IS_ALIGNED(addr, align) (((addr) & ((align) - 1)) == 0)

const char *ANSI_RESET;
const char *ANSI_BLACK;
const char *ANSI_RED;
const char *ANSI_GREEN;
const char *ANSI_YELLOW;
const char *ANSI_BLUE;
const char *ANSI_PURPLE;
const char *ANSI_CYAN;
const char *ANSI_WHITE;

const char *LINUX_ERRNO[37];

const char *SIGNALS[34];
const char *signalStr(int signum);

void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size);
void *memmove(void *dstptr, const void *srcptr, size_t size);
void  memset(void *_dst, int val, size_t len);
int   memcmp(const void *aptr, const void *bptr, size_t size);

void     atomicBitmapSet(volatile uint64_t *bitmap, unsigned int bit);
void     atomicBitmapClear(volatile uint64_t *bitmap, unsigned int bit);
uint64_t atomicBitmapGet(volatile uint64_t *bitmap);

int  rand(void);
void srand(unsigned int seed);
void hexDump(const char *desc, const void *addr, const int len, int perLine,
             int (*f)(const char *fmt, ...));

#endif
