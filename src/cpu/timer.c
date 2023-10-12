#include "../../include/timer.h"

void initiateTimer(uint32_t reload_value) {
  uint32_t frequency = TIMER_ACCURANCY / reload_value;

  outportb(0x43, 0x36);

  uint8_t l = (uint8_t)(frequency & 0xFF);
  uint8_t h = (uint8_t)(frequency >> 8 & 0xFF);

  outportb(0x40, l);
  outportb(0x40, h);
}

void timerTick() { schedule(); }