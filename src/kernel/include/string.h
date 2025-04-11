#ifndef STRING_H
#define STRING_H

#include "types.h"

bool   strEql(char *ch1, char *ch2);
char  *strtok(char *str, const char *delimiters, char **context);
char  *strpbrk(const char *str, const char *delimiters);
int    atoi(const char *str);
size_t strlength(const char *ch);
size_t strlen(const char *ch);
int    strncmp(const char *str1, const char *str2, size_t n);
bool   check_string(char *str);
long   strtol(const char *s, char **endptr, int base);
void   strncpy(char *dest, const char *src, size_t n);
char  *strdup(char *source);
char  *strrchr(const char *str, int c);

int isdigit(char c);

#endif
