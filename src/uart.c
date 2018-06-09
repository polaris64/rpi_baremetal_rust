#include <stddef.h>
#include <stdint.h>

#include "utility.h"
#include "uart.h"
#include "gpio.h"

void uart_init(void)
{
  // Disables all aspects of UART using CR
  mmio_write(UART0_CR, 0x0);

  // Disable GPIO pins: writing 0 to GPPUD marks that pins should be disabled,
  // and GPPUDCLK0 marks which pins. Finally, writing 0 to GPPUDCLK0 fialises
  // the changes.
  mmio_write(GPPUD, 0x0);
  delay(150);
  mmio_write(GPPUDCLK0, ((1 << 14) | (1 << 15)));
  delay(150);
  mmio_write(GPPUDCLK0, 0x0);

  // Set all flags in the Interrupt Clear Register (clear all pending
  // interrupts)
  mmio_write(UART0_ICR, 0x7ff);

  // Set baud rate to 115200. Divisor is UART_CLOCK_SPEED / (16 * BAUD). The
  // integer part is stored in UART0_IBRD while the fractional portion is
  // stored in UART0_FBRD (FRAC * 64) + 0.5.  Result ends up being ~1.67, so
  // UART0_IBRD is 1 and UART0_FBRD is (0.67 * 64) + 0.5 = 43.
  mmio_write(UART0_IBRD,  1);
  mmio_write(UART0_FBRD, 43);

  // Write bit 4, 5 and 6 to Line Control Register: -
  //   - Bit 4: use 8 item deep FIFO instead of single item register
  //   - Bits 5 and 6: use 8-bit words for RX/TX
  mmio_write(UART0_LCRH, (1 << 4) | (1 << 5) | (1 << 6));

  // Disable all interrupts from UART0 by setting relevant bits in Interrupt
  // Mask Set Clear register
  mmio_write(UART0_IMSC, (1 << 1) | (1 << 4) | (1 << 5) | (1 << 6) | (1 << 7) | (1 << 8) | (1 << 9) | (1 << 10));

  // Bit 0: enable UART0 hardware
  // Bit 8: enable RX
  // Bit 9: enable TX
  mmio_write(UART0_CR, (1 << 0) | (1 << 8) | (1 << 9));
}

void uart_putc(unsigned char ch)
{
  // Loop until flag register bit 5 (TXFF: transmit FIFO is full) is unset
  while (mmio_read(UART0_FR) & (1 << 5)) {}

  // Write character to data register
  mmio_write(UART0_DR, ch);
}

unsigned char uart_getc(void)
{
  // Loop until flag register bit 4 (RXFE: receive FIFO is empty) is unset
  while (mmio_read(UART0_FR) & (1 << 4)) {}

  // Read character from data register and return
  return mmio_read(UART0_DR);
}

void uart_puts(const char *str)
{
  for (size_t i = 0; str[i] != '\0'; i++)
    uart_putc((unsigned char)str[i]);
}
