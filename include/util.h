#ifndef UTIL_H
#define UTIL_H

#include "types.h"

#define DIV_ROUND_CLOSEST(n, d)                                                \
  ((((n) < 0) == ((d) < 0)) ? (((n) + (d) / 2) / (d)) : (((n) - (d) / 2) / (d)))

void   memory_copy(char *source, char *dest, int nbytes);
void   memory_set(uint8 *dest, uint8 val, uint32 len);
string int_to_ascii(int n, char str[]);
int    str_to_int(string ch);
string char_to_string(char ch);
uint8  check_string_numbers(string str);
void  *malloc(int nbytes);
uint8  check_string(string str);

#endif
