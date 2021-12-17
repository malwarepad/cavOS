#ifndef UTIL_H
#define UTIL_H

#include "types.h"

void memory_copy(char *source, char *dest, int nbytes);
void memory_set(uint8 *dest, uint8 val, uint32 len);
void int_to_ascii(int n, char str[]);  
int str_to_int(string ch)  ;
string char_to_string(char ch);
uint8 check_string_numbers(string str);
uint8 string_to_int_corr(const char *str);
void * malloc(int nbytes);      

#endif
