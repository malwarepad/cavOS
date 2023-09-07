#include "../../../include/string.h"
#include "../../../include/tty.h"
#include "../../../include/util.h"

// String management file
// Copyright (C) 2023 Panagiotis

uint16 strlength(string ch) {
  uint16 i = 0; // Changed counter to 0
  while (ch[i++])
    ;
  return i - 1; // Changed counter to i instead of i--
}

uint8 cmdLength(string ch) {
  uint8 i = 0;
  uint8 size = 0;
  uint8 realSize = strlength(ch);
  uint8 hasSpace = 0;

  string space;
  space[0] = ' ';

  for (i; i <= realSize; i++) {
    if (hasSpace == 0) {
      if (ch[i] != space[0]) {
        size++;
      } else {
        hasSpace = 1;
      }
    }
  }

  if (hasSpace != 1) {
    size--;
  }

  size--;
  return size;
}

uint8 isStringEmpty(string ch1) {
  if (ch1[0] == '1') {
    return 0;
  } else if (ch1[0] == '2') {
    return 0;
  } else if (ch1[0] == '3') {
    return 0;
  } else if (ch1[0] == '4') {
    return 0;
  } else if (ch1[0] == '5') {
    return 0;
  } else if (ch1[0] == '6') {
    return 0;
  } else if (ch1[0] == '7') {
    return 0;
  } else if (ch1[0] == '8') {
    return 0;
  } else if (ch1[0] == '9') {
    return 0;
  } else if (ch1[0] == '0') {
    return 0;
  } else if (ch1[0] == '-') {
    return 0;
  } else if (ch1[0] == '=') {
    return 0;
  } else if (ch1[0] == 'q') {
    return 0;
  } else if (ch1[0] == 'w') {
    return 0;
  } else if (ch1[0] == 'e') {
    return 0;
  } else if (ch1[0] == 'r') {
    return 0;
  } else if (ch1[0] == 't') {
    return 0;
  } else if (ch1[0] == 'y') {
    return 0;
  } else if (ch1[0] == 'u') {
    return 0;
  } else if (ch1[0] == 'i') {
    return 0;
  } else if (ch1[0] == 'o') {
    return 0;
  } else if (ch1[0] == 'p') {
    return 0;
  } else if (ch1[0] == '[') {
    return 0;
  } else if (ch1[0] == ']') {
    return 0;
  } else if (ch1[0] == '\\') {
    return 0;
  } else if (ch1[0] == 'a') {
    return 0;
  } else if (ch1[0] == 's') {
    return 0;
  } else if (ch1[0] == 'd') {
    return 0;
  } else if (ch1[0] == 'f') {
    return 0;
  } else if (ch1[0] == 'g') {
    return 0;
  } else if (ch1[0] == 'h') {
    return 0;
  } else if (ch1[0] == 'j') {
    return 0;
  } else if (ch1[0] == 'k') {
    return 0;
  } else if (ch1[0] == 'l') {
    return 0;
  } else if (ch1[0] == ';') {
    return 0;
  } else if (ch1[0] == '\'') {
    return 0;
  } else if (ch1[0] == 'z') {
    return 0;
  } else if (ch1[0] == 'x') {
    return 0;
  } else if (ch1[0] == 'c') {
    return 0;
  } else if (ch1[0] == 'v') {
    return 0;
  } else if (ch1[0] == 'b') {
    return 0;
  } else if (ch1[0] == 'n') {
    return 0;
  } else if (ch1[0] == 'm') {
    return 0;
  } else if (ch1[0] == ',') {
    return 0;
  } else if (ch1[0] == '.') {
    return 0;
  } else if (ch1[0] == '/') {
    return 0;
  } else {
    return 1;
  }
}

uint8 strEql(string ch1, string ch2) {
  uint8 result = 1;
  uint8 size = strlength(ch1);
  if (size != strlength(ch2))
    result = 0;
  else {
    uint8 i = 0;
    for (i; i <= size; i++) {
      if (ch1[i] != ch2[i])
        result = 0;
    }
  }
  return result;
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

int charAppearance(string target, char charToAppear) {
  int count = 0;
  for (int i = 0; i < strlength(target); i++) {
    if (target[i] == charToAppear)
      count++;
  }
  return count;
}
