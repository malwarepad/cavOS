#ifndef UTIL_H
#define UTIL_H

#include "allocation.h"
#include "multiboot.h"
#include "types.h"

#define DIV_ROUND_CLOSEST(n, d)                                                \
  ((((n) < 0) == ((d) < 0)) ? (((n) + (d) / 2) / (d)) : (((n) - (d) / 2) / (d)))

void   memset(void *_dst, int val, size_t len);
void   memory_copy(char *source, char *dest, int nbytes);
void   memory_set(uint8 *dest, uint8 val, uint32 len);
string int_to_ascii(int n, char str[]);
int    str_to_int(string ch);
string char_to_string(char ch);
uint8  check_string_numbers(string str);
uint8  check_string(string str);
int    memcmp(const void *aptr, const void *bptr, size_t size);

#endif
