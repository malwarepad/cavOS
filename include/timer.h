#include "schedule.h"
#include "types.h"

#define TIMER_ACCURANCY 1193180

#ifndef TIMER_H
#define TIMER_H

void initiateTimer(uint32_t reload_value);
void timerTick();

#endif