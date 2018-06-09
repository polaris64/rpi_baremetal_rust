#ifndef UART_H
#define UART_H

#include "peripherals.h"
#include "gpio.h"

#define UART0_BASE PERIPHERAL_BASE + GPIO_BASE + 0x1000

#define UART0_DR     UART0_BASE + 0x00 // Data register
#define UART0_RSRECR UART0_BASE + 0x04 // Read status register
#define UART0_FR     UART0_BASE + 0x18 // Flag register
#define UART0_IBRD   UART0_BASE + 0x24 // Integer Baud rate divisor
#define UART0_FBRD   UART0_BASE + 0x28 // Fractional Baud rate divisor
#define UART0_LCRH   UART0_BASE + 0x2c // Line control register
#define UART0_CR     UART0_BASE + 0x30 // Control register
#define UART0_IMSC   UART0_BASE + 0x38 // Interrupt mask set clear register
#define UART0_ICR    UART0_BASE + 0x44 // Interrupt clear register
// ...

void          uart_init(void);
void          uart_putc(unsigned char);
unsigned char uart_getc(void);
void          uart_puts(const char*);

#endif
