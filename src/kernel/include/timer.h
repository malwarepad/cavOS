#include "types.h"

#define TIMER_ACCURANCY 1193182

#ifndef TIMER_H
#define TIMER_H

uint32_t timerFrequency;
uint64_t timerTicks;
uint64_t timerBootUnix;

void     initiateTimer(uint32_t reload_value);
void     timerTick(uint64_t rsp);
uint32_t sleep(uint32_t time);

#endif