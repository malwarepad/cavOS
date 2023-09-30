#ifndef STRING_H
#define STRING_H

#include "types.h"

uint16 strlength(string ch);
uint8  strEql(string ch1, string ch2);
uint8  cmdLength(string ch);
uint8  isStringEmpty(string ch1);
char  *strtok(char *str, const char *delimiters, char **context);
char  *strpbrk(const char *str, const char *delimiters);
int    charAppearance(string target, char charToAppear);
int    atoi(const char *str);

#endif
