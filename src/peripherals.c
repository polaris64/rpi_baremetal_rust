#include <stdint.h>

#include "peripherals.h"

/*
 * General-purpose memory mapped IO (MMIO) read/write functions
 */

inline void mmio_write(uint32_t reg, uint32_t data)
{
  *(volatile uint32_t*)reg = data;
}

inline uint32_t mmio_read(uint32_t reg)
{
  return *(volatile uint32_t*)reg;
}
