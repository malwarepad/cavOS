#include "types.h"

#define TIMER_ACCURANCY 1193182

#ifndef TIMER_H
#define TIMER_H

uint32_t timerFrequency;
uint64_t timerTicks;

void initiateTimer(uint32_t reload_value);
void timerTick(uint64_t rsp);
void sleep(uint32_t time);

#endif