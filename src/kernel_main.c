#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "utility.h"
#include "gpio.h"
#include "timers.h"
#include "framebuffer.h"
#include "uart.h"

volatile FramebufferInfo fbinfo;

void death_loop(uint32_t min, uint32_t max, uint32_t step)
{
  bool     led_on     = true;
  uint32_t wait_time  = min;
  int32_t  wait_delta = step;

  // Set GPIO pin 47 (ACT LED) to "write"
  setGpioFunction(47, 1);

  while (1)
  {
    // Set/unset pin output
    setGpio(47, led_on);

    // Wait for a bit...
    delay(wait_time);

    // Toggle led_on
    led_on = !led_on;

    // Update wait_time
    wait_time += wait_delta;
    if (wait_time < min || wait_time > max)
    {
      wait_time = wait_delta > 0 ? max : min;
      wait_delta = wait_delta > 0 ? -step : step;
    }
  }
}

void kernel_main(uint32_t r0, uint32_t r1, uint32_t atags)
{
  (void) r0;
  (void) r1;
  (void) atags;
  unsigned char ch = 0;

  // Initialise the framebuffer (1024x768x32)
  if (!initFramebufferInfo(&fbinfo, 1024, 768, 32))
    death_loop(0x1000, 0x10000, 0x500);

  if (!initFramebuffer(&fbinfo))
    death_loop(0x1000, 0x20000, 0x1000);

  uart_init();
  uart_puts("Press keys to change tests pattern, press 'q' to halt");

  while (1)
  {
    drawFBTestPattern(fbinfo, ch);
    
    ch = uart_getc();
    if (ch == 'q')
      break;
    uart_putc(ch);
    uart_putc('\n');
  }

  uart_puts("Halting...\n");

  // Returning from kernel_main returns to instruction after BL kernel_main in
  // main.S, which is the CPU core halt loop
}
