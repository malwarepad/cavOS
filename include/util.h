#ifndef UTIL_H
#define UTIL_H

#include "multiboot2.h"
#include "types.h"

#define DIV_ROUND_CLOSEST(n, d)                                                \
  ((((n) < 0) == ((d) < 0)) ? (((n) + (d) / 2) / (d)) : (((n) - (d) / 2) / (d)))

#define DivRoundUp(number, divisor) ((number + divisor - 1) / divisor)
#define inrand(minimum_number, max_number)                                     \
  (rand() % (max_number + 1 - minimum_number) + minimum_number)

void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size);
void *memmove(void *dstptr, const void *srcptr, size_t size);
void  memset(void *_dst, int val, size_t len);
void  memory_copy(char *source, char *dest, int nbytes);
void  memory_set(uint8 *dest, uint8 val, uint32 len);
uint8 check_string(string str);
int   memcmp(const void *aptr, const void *bptr, size_t size);
int   rand(void);
void  srand(unsigned int seed);

#endif
