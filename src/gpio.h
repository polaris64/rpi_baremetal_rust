#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>

#define GPIO_BASE 0x200000UL

#define GPPUD     GPIO_BASE + 0x94 // GPIO pin pull-up/down enable
#define GPPUDCLK0 GPIO_BASE + 0x98 // GPIO pin pull-up/down enable clock 0
#define GPPUDCLK1 GPIO_BASE + 0x9c // GPIO pin pull-up/down enable clock 1

uint32_t getGpioAddress(void);
void     setGpioFunction(uint8_t pin, uint8_t cmd);
void     setGpio(uint8_t pin, bool value);

#endif
