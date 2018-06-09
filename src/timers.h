#ifndef TIMERS_H
#define TIMERS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "peripherals.h"

#define TIMER_BASE 0x3000UL

uint32_t getTimerAddress(void);
uint32_t getTimerLo(void);
uint32_t getTimerHi(void);

#endif
