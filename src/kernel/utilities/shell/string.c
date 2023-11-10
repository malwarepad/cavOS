#include "../../include/string.h"
#include "../../include/util.h"

// String management file
// Copyright (C) 2023 Panagiotis

uint32_t strlength(char *ch) {
  uint32_t i = 0; // Changed counter to 0
  while (ch[i++])
    ;
  return i - 1; // Changed counter to i instead of i--
}

int atoi(const char *str) {
  int value = 0;
  while (isdigit(*str)) {
    value *= 10;
    value += (*str) - '0';
    str++;
  }

  return value;
}

bool check_string(char *str) { return (str[0] != 0x0); }

bool strEql(char *ch1, char *ch2) {
  uint32_t size = strlength(ch1);

  if (size != strlength(ch2))
    return false;

  for (uint32_t i = 0; i <= size; i++) {
    if (ch1[i] != ch2[i])
      return false;
  }

  return true;
}

char *strpbrk(const char *str, const char *delimiters) {
  while (*str) {
    const char *d = delimiters;
    while (*d) {
      if (*d == *str) {
        return (char *)str;
      }
      ++d;
    }
    ++str;
  }
  return NULL;
}

char *strtok(char *str, const char *delimiters, char **context) {
  if (str == NULL && *context == NULL)
    return NULL;

  if (str != NULL)
    *context = str;

  char *token_start = *context;
  char *token_end = strpbrk(token_start, delimiters);

  if (token_end != NULL) {
    *token_end = '\0';
    *context = token_end + 1;
    return token_start;
  } else if (*token_start != '\0') {
    *context = NULL;
    return token_start;
  }

  return NULL;
}
