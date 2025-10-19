#include "types.h"

#ifndef RTC_H
#define RTC_H

typedef struct {
  unsigned char second;
  unsigned char minute;
  unsigned char hour;
  unsigned char day;
  unsigned char month;
  unsigned int  year;
} RTC;

int      readFromCMOS(RTC *rtc);
uint64_t rtcToUnix(RTC *rtc);
bool     isLeapYear(int year);

int century_register;

#endif
