#include "timers.h"

uint32_t getTimerAddress(void)
{
  return PERIPHERAL_BASE | TIMER_BASE;
}

uint32_t getTimerLo(void)
{
  return *((uint32_t*)getTimerAddress() + 0x04);
}

uint32_t getTimerHi(void)
{
  return *((uint32_t*)getTimerAddress() + 0x08);
}
