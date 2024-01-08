#include <timer.h>

void initiateTimer(uint32_t reload_value) {
  timerTicks = 0;
  timerFrequency = TIMER_ACCURANCY / reload_value;

  outportb(0x43, 0x36);

  uint8_t l = (uint8_t)(timerFrequency & 0xFF);
  uint8_t h = (uint8_t)(timerFrequency >> 8 & 0xFF);

  outportb(0x40, l);
  outportb(0x40, h);
  debugf("[timer] Ready to fire: frequency{%dMHz}\n", timerFrequency);
}

void timerTick() {
  timerTicks++;
  schedule();
}

void sleep(uint32_t time) {
  uint64_t target = timerTicks + (time);
  while (target > timerTicks) {
  }
}