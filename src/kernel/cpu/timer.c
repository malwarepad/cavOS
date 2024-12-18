#include <isr.h>
#include <rtc.h>
#include <schedule.h>
#include <system.h>
#include <timer.h>

void initiateTimer(uint32_t reload_value) {
  timerFrequency = TIMER_ACCURANCY / reload_value;

  outportb(0x43, 0x36);

  uint8_t l = (uint8_t)(timerFrequency & 0xFF);
  uint8_t h = (uint8_t)(timerFrequency >> 8 & 0xFF);

  outportb(0x40, l);
  outportb(0x40, h);

  RTC rtc = {0};
  readFromCMOS(&rtc);
  timerTicks = 0;

  timerBootUnix = rtcToUnix(&rtc);
  debugf("[timer] Ready to fire: frequency{%dMHz}\n", timerFrequency);
}

void timerTick(uint64_t rsp) {
  timerTicks++;
  schedule(rsp);
}

uint32_t sleep(uint32_t time) {
  uint64_t target = timerTicks + (time);
  while (target > timerTicks)
    handControl();

  return 0;
}