#ifndef STRING_H
#define STRING_H

#include "types.h"

bool     strEql(char *ch1, char *ch2);
char    *strtok(char *str, const char *delimiters, char **context);
char    *strpbrk(const char *str, const char *delimiters);
int      atoi(const char *str);
uint32_t strlength(char *ch);
bool     check_string(char *str);

#endif
