#ifndef STRING_H
#define STRING_H

#include "types.h"
uint16 strlength(string ch);

uint8 strEql(string ch1,string ch2);
uint8 cmdEql(string ch1, string ch2);
uint8 cmdLength(string ch);
uint8 whereSpace1(string ch1);
uint8 searchArgMain(string ch1, string ch2);
uint8 whatIsArgMain(string ch1);
uint8 isStringEmpty(string ch1);

#endif
