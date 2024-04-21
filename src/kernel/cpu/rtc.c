#include <rtc.h>
#include <system.h>

// Real-Time Clock implementation
// Copyright (C) 2024 Panagiotis

// most code from the excellent https://wiki.osdev.org/CMOS#The_Real-Time_Clock
// reference

// Change this each year!
#define CURRENT_YEAR 2024

int century_register = 0x00; // Set by ACPI table parsing code if possible

/*unsigned char second;
unsigned char minute;
unsigned char hour;
unsigned char day;
unsigned char month;
unsigned int  year;*/

enum { cmos_address = 0x70, cmos_data = 0x71 };

int get_update_in_progress_flag() {
  outportb(cmos_address, 0x0A);
  return (inportb(cmos_data) & 0x80);
}

unsigned char get_RTC_register(int reg) {
  outportb(cmos_address, reg);
  return inportb(cmos_data);
}

int readFromCMOS(RTC *rtc) {
  unsigned char century;
  unsigned char last_second;
  unsigned char last_minute;
  unsigned char last_hour;
  unsigned char last_day;
  unsigned char last_month;
  unsigned char last_year;
  unsigned char last_century;
  unsigned char registerB;

  // Note: This uses the "read registers until you get the same values twice in
  // a row" technique
  //       to avoid getting dodgy/inconsistent values due to RTC updates

  while (get_update_in_progress_flag())
    ; // Make sure an update isn't in progress
  rtc->second = get_RTC_register(0x00);
  rtc->minute = get_RTC_register(0x02);
  rtc->hour = get_RTC_register(0x04);
  rtc->day = get_RTC_register(0x07);
  rtc->month = get_RTC_register(0x08);
  rtc->year = get_RTC_register(0x09);
  if (century_register != 0) {
    century = get_RTC_register(century_register);
  }

  do {
    last_second = rtc->second;
    last_minute = rtc->minute;
    last_hour = rtc->hour;
    last_day = rtc->day;
    last_month = rtc->month;
    last_year = rtc->year;
    last_century = century;

    while (get_update_in_progress_flag())
      ; // Make sure an update isn't in progress
    rtc->second = get_RTC_register(0x00);
    rtc->minute = get_RTC_register(0x02);
    rtc->hour = get_RTC_register(0x04);
    rtc->day = get_RTC_register(0x07);
    rtc->month = get_RTC_register(0x08);
    rtc->year = get_RTC_register(0x09);
    if (century_register != 0) {
      century = get_RTC_register(century_register);
    }
  } while ((last_second != rtc->second) || (last_minute != rtc->minute) ||
           (last_hour != rtc->hour) || (last_day != rtc->day) ||
           (last_month != rtc->month) || (last_year != rtc->year) ||
           (last_century != century));

  registerB = get_RTC_register(0x0B);

  // Convert BCD to binary values if necessary

  if (!(registerB & 0x04)) {
    rtc->second = (rtc->second & 0x0F) + ((rtc->second / 16) * 10);
    rtc->minute = (rtc->minute & 0x0F) + ((rtc->minute / 16) * 10);
    rtc->hour = ((rtc->hour & 0x0F) + (((rtc->hour & 0x70) / 16) * 10)) |
                (rtc->hour & 0x80);
    rtc->day = (rtc->day & 0x0F) + ((rtc->day / 16) * 10);
    rtc->month = (rtc->month & 0x0F) + ((rtc->month / 16) * 10);
    rtc->year = (rtc->year & 0x0F) + ((rtc->year / 16) * 10);
    if (century_register != 0) {
      century = (century & 0x0F) + ((century / 16) * 10);
    }
  }

  // Convert 12 hour clock to 24 hour clock if necessary

  if (!(registerB & 0x02) && (rtc->hour & 0x80)) {
    rtc->hour = ((rtc->hour & 0x7F) + 12) % 24;
  }

  // Calculate the full (4-digit) year

  if (century_register != 0) {
    rtc->year += century * 100;
  } else {
    rtc->year += (CURRENT_YEAR / 100) * 100;
    if (rtc->year < CURRENT_YEAR)
      rtc->year += 100;
  }

  return 1;
}