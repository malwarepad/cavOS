#ifndef STRING_H
#define STRING_H

#include "types.h"

bool     strEql(char *ch1, char *ch2);
char    *strtok(char *str, const char *delimiters, char **context);
char    *strpbrk(const char *str, const char *delimiters);
int      atoi(const char *str);
uint32_t strlength(const char *ch);
bool     check_string(char *str);
long     strtol(const char *s, char **endptr, int base);

#endif
