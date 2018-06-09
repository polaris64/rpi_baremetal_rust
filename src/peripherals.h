#ifndef PERIPHERALS_H
#define PERIPHERALS_H

// 0x20000000 on RPi 1
#define PERIPHERAL_BASE 0x3F000000

void     mmio_write(uint32_t, uint32_t);
uint32_t mmio_read(uint32_t);

#endif
