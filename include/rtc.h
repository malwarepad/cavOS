#include "system.h"
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

int readFromCMOS(RTC *rtc);

#endif
