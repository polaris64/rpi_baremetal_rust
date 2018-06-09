#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gpio.h"
#include "peripherals.h"

uint32_t getGpioAddress(void)
{
  return PERIPHERAL_BASE | GPIO_BASE;
}

void setGpioFunction(uint8_t pin, uint8_t cmd)
{
  uint32_t *gpio_gpfsel = 0;
  uint32_t  cmd_bits    = 0;

  /*
   * Validate arguments
   *  - pin must be 0..53
   *  - cmd must be 0..7 (3 bits)
   */
  if (pin > 53)
    return;
  if (cmd > 7)
    return;

  /*
   * Find correct function select register for pin. Each register is 32
   * bits and each pin occupies 3 bits. Each register can therefore
   * contain the state for 10 pins (30 bits) and the highest 2 bits are
   * unused.
   *
   * The pin number is divided by 10 to find the correct register (GPFSELn)
   * which is then added to the GPIO base address.
   */
  gpio_gpfsel = ((uint32_t*)getGpioAddress()) + (pin / 10);

  /*
   * The three bits of cmd should be written to the GPFSELn register at the
   * correct offset, so the pin number modulo 10 is multiplied by 3 (3 bits per
   * pin) to find the offset. cmd is then shifted left by this offset.
   */
  cmd_bits = (uint32_t)cmd << ((pin % 10) * 3);

  /*
   * Here the cmd bits (32 bits) are written directly to the GPFSELn register.
   * THIS IS BAD! Any existing register bits will be overwritten.
   * TODO: fix setGpioFunction()
   */
  (*gpio_gpfsel) = cmd_bits;
}

void setGpio(uint8_t pin, bool value)
{
  uint8_t  gpio_reg_offset = 0;
  uint8_t *gpio_reg        = 0;

  // pin must be 0..53
  if (pin > 53)
    return;

  /*
   * The GPIO set/clear registers (GPSETn, GPCLRn) are split into two banks
   * each. The first is used for pins 0..31 and the second for pins 32..53).
   * The actual register is BASE + 0x1C (GPSET0), BASE + 0x20 (GPSET1), BASE +
   * 0x28 (GPCLR0) and BASE + 0x2C (GPCLR1)
   */

  /*
   * Calculate which register should be used (0 or 1) by dividing pin by 32 and
   * multiplying by 4 (each register is 32 bits long and 8 bit pointers
   * are being used).
   */
  gpio_reg_offset = (pin / 32) * 4;

  /*
   * Having found the correct register offset, the correct register GPSETn or
   * GPCLRn needs to be found based on "value" (SET if TRUE, CLR if FALSE).
   * Here, the actual register address will be calculated.
   */
  gpio_reg = ((uint8_t*)getGpioAddress()) + (value ? 0x1C : 0x28) + gpio_reg_offset;

  /*
   * The correct bit now needs to be found within the register. This is the pin
   * number modulo 32. Once found, a 1 bit is written to the register at that
   * position.
   */
  (*((uint32_t*)gpio_reg)) |= 1 << (pin % 32);
}
